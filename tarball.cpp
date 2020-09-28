/*
 * tarball.cpp
 *
 *  Created on: Jul 28, 2010
 *      Author: Pierre Lindenbaum PhD
 *              plindenbaum@yahoo.fr
 *              http://plindenbaum.blogspot.com
 *
 *  Modified on: Sep 22, 2020
 *		Author: Ravbug (https://www.ravbug.com)
 *
 * Including some code by Uli Köhler
 * 		(https://techoverflow.net/2013/03/29/reading-tar-files-in-c/)
 */
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <ctime>
#include <tarball.hpp>
#include <sstream>
#define TARHEADER static_cast<PosixTarHeader*>(header)

using namespace Tar;
using namespace std;

#define ASCII_TO_NUMBER(num) ((num)-48) //Converts an ascii digit to the corresponding number (assuming it is an ASCII digit)

/**
 * Decode a TAR octal number.
 * Ignores everything after the first NUL or space character.
 * @param data A pointer to a size-byte-long octal-encoded
 * @param size The size of the field pointer to by the data pointer
 * @return
 */
static uint64_t decodeTarOctal(char* data, size_t size = 12) {
	unsigned char* currentPtr = (unsigned char*) data + size;
	uint64_t sum = 0;
	uint64_t currentMultiplier = 1;
	//Skip everything after the last NUL/space character
	//In some TAR archives the size field has non-trailing NULs/spaces, so this is neccessary
	unsigned char* checkPtr = currentPtr; //This is used to check where the last NUL/space char is
	for (; checkPtr >= (unsigned char*) data; checkPtr--) {
		if ((*checkPtr) == 0 || (*checkPtr) == ' ') {
			currentPtr = checkPtr - 1;
		}
	}
	for (; currentPtr >= (unsigned char*) data; currentPtr--) {
		sum += ASCII_TO_NUMBER(*currentPtr) * currentMultiplier;
		currentMultiplier *= 8;
	}
	return sum;
}

struct PosixTarHeader
{
	char name[100];
	char mode[8];
	char uid[8];
	char gid[8];
	char size[12];
	char mtime[12];
	char checksum[8];
	char typeflag;
	char linkname[100];
	char magic[6];
	char version[2];
	char uname[32];
	char gname[32];
	char devmajor[8];
	char devminor[8];
	char prefix[155];
	char pad[12];
		
	/**
	 * @return true if and only if
	 */
	bool isUSTAR() {
		return (memcmp("ustar", magic, 5) == 0);
	}
	/**
	 * @return The filesize in bytes
	 */
	size_t getFileSize() {
		return decodeTarOctal(size);
	}
	
	/**
	 * Return true if and only if the header checksum is correct
	 * @return
	 */
	bool checkChecksum() {
		//We need to set the checksum to zer
		char originalChecksum[8];
		memcpy(originalChecksum, checksum, 8);
		memset(checksum, ' ', 8);
		//Calculate the checksum -- both signed and unsigned
		int64_t unsignedSum = 0;
		int64_t signedSum = 0;
		for (int i = 0; i < sizeof (PosixTarHeader); i++) {
			unsignedSum += ((unsigned char*) this)[i];
			signedSum += ((signed char*) this)[i];
		}
		//Copy back the checksum
		memcpy(checksum, originalChecksum, 8);
		//Decode the original checksum
		uint64_t referenceChecksum = decodeTarOctal(originalChecksum);
		return (referenceChecksum == unsignedSum || referenceChecksum == signedSum);
	}
};



void TarWriter::_init(void* header)
{
    std::memset(header,0,sizeof(PosixTarHeader));
    std::strcpy(TARHEADER->magic,"ustar");
    std::strcpy(TARHEADER->version, " ");
//    std::sprintf(TARHEADER->mtime,"%011lo",time(NULL));
//    std::sprintf(TARHEADER->mode,"%07o",0644);
//    char * s = ::getlogin();
//    if(s!=NULL)  std::snprintf(TARHEADER,32,"%s",s);
//    std::sprintf(TARHEADER->gname,"%s","users");
}

void TarWriter::_checksum(void* header)
{
    unsigned int sum = 0;
    char *p = (char *) header;
    char *q = p + sizeof(PosixTarHeader);
    while (p < TARHEADER->checksum) sum += *p++ & 0xff;
    for (int i = 0; i < 8; ++i)  {
	  sum += ' ';
	  ++p;
}
    while (p < q) sum += *p++ & 0xff;

    std::sprintf(TARHEADER->checksum,"%06o",sum);
}

void TarWriter::_size(void* header,unsigned long fileSize)
{
    std::sprintf(TARHEADER->size,"%011llo",(long long unsigned int)fileSize);
}

void TarWriter::_filename(void* header,const char* filename)
{
    if(filename==NULL || filename[0]==0 || std::strlen(filename)>=100)
	{
		throw runtime_error(string("invalid archive name \"") + filename + "\"");
	}
    std::snprintf(TARHEADER->name,100,"%s",filename);
}

void TarWriter::_endRecord(std::size_t len)
{
    char c='\0';
    while((len%sizeof(PosixTarHeader))!=0)
	{
	    out.write(&c,sizeof(char));
	    ++len;
	}
}


TarWriter::TarWriter(std::ostream& out):_finished(false),out(out)
{
    if(sizeof(PosixTarHeader)!=512)
	{
		throw runtime_error(string("Incorrect header size: 512 != ") + to_string(sizeof(PosixTarHeader)));
	}
}

TarWriter::~TarWriter()
{
    if(!_finished)
	{
		cerr << "[warning]tar file was not finished."<< endl;
	}
}

/** writes 2 empty blocks. Should be always called before closing the Tar file */
void TarWriter::finish()
{
    _finished=true;
    //The end of the archive is indicated by two blocks filled with binary zeros
    PosixTarHeader header;
    std::memset((void*)&header,0,sizeof(PosixTarHeader));
    out.write((const char*)&header,sizeof(PosixTarHeader));
    out.write((const char*)&header,sizeof(PosixTarHeader));
    out.flush();
}

void TarWriter::put(const char* filename,const std::string& s)
{
    put(filename,s.c_str(),s.size());
}
void TarWriter::put(const char* filename,const char* content)
{
	put(filename,content,std::strlen(content));
}

void TarWriter::put(const char* filename,const char* content,std::size_t len)
{
    PosixTarHeader header;
    _init((void*)&header);
    _filename((void*)&header,filename);
    header.typeflag = 0;
    _size((void*)&header,len);
    _checksum((void*)&header);
    out.write((const char*)&header,sizeof(PosixTarHeader));
    out.write(content,len);
    _endRecord(len);
}

void TarWriter::putFile(const char* filename,const char* nameInArchive)
{
    char buff[BUFSIZ];
    std::FILE* in=std::fopen(filename,"rb");
    if(in==NULL)
	{
		throw runtime_error(string("Cannot open ") + filename + " " + std::strerror(errno));
	}
    std::fseek(in, 0L, SEEK_END);
    long int len= std::ftell(in);
    std::fseek(in,0L,SEEK_SET);

    PosixTarHeader header;
    _init((void*)&header);
    _filename((void*)&header,nameInArchive);
    header.typeflag = 0;
    _size((void*)&header,len);
    _checksum((void*)&header);
    out.write((const char*)&header,sizeof(PosixTarHeader));


    std::size_t nRead=0;
    while((nRead=std::fread(buff,sizeof(char),BUFSIZ,in))>0)
	{
		out.write(buff,nRead);
	}
    std::fclose(in);

    _endRecord(len);
}


TarReader::TarReader(istream& in){
	//Initialize a zero-filled block we can compare against (zero-filled header block --> end of TAR archive)
	char zeroBlock[512];
	memset(zeroBlock, 0, 512);
	//Start reading
	bool nextEntryHasLongName = false;
	while (in) { //Stop if end of file has been reached or any error occured
		PosixTarHeader currentFileHeader;
		//Read the file header.
		in.read((char*) &currentFileHeader, 512);
		//When a block with zeroes-only is found, the TAR archive ends here
		if(memcmp(&currentFileHeader, zeroBlock, 512) == 0) {
			//found TAR end
			break;
		}
		//Uncomment this to check all header checksums
		//There seem to be TARs on the internet which include single headers that do not match the checksum even if most headers do.
		//This might indicate a code error.
		//assert(currentFileHeader.checkChecksum());
		//Uncomment this to check for USTAR if you need USTAR features
		//assert(currentFileHeader.isUSTAR());
		//Convert the filename to a std::string to make handling easier
		//Filenames of length 100+ need special handling
		// (only USTAR supports 101+-character filenames, but in non-USTAR archives the prefix is 0 and therefore ignored)
		string filename(currentFileHeader.name, min((size_t)100, strlen(currentFileHeader.name)));
		//---Remove the next block if you don't want to support long filenames---
		size_t prefixLength = strlen(currentFileHeader.prefix);
		if(prefixLength > 0) { //If there is a filename prefix, add it to the string. See `man ustar`LON
			filename = string(currentFileHeader.prefix, min((size_t)155, prefixLength)) + "/" + filename; //min limit: Not needed by spec, but we want to be safe
		}
		//Ignore directories, only handle normal files (symlinks are currently ignored completely and might cause errors)
		if (currentFileHeader.typeflag == '0' || currentFileHeader.typeflag == 0) { //Normal file
			//Handle GNU TAR long filenames -- the current block contains the filename only whilst the next block contains metadata
			if(nextEntryHasLongName) {
				//Set the filename from the current header
				filename = string(currentFileHeader.name);
				//The next header contains the metadata, so replace the header before reading the metadata
				in.read((char*) &currentFileHeader, 512);
				//Reset the long name flag
				nextEntryHasLongName = false;
			}
			//Now the metadata in the current file header is valie -- we can read the values.
			size_t size = currentFileHeader.getFileSize();
			//Read the file into memory
			ostringstream str;
			for(int i = 0; i < size; i++){
				char byte;
				in.get(byte);
				str << byte;
			}
			
			//insert into the map
			files.insert(make_pair(filename, str.str()));
			
			//In the tar archive, entire 512-byte-blocks are used for each file
			//Therefore we now have to skip the padded bytes.
			size_t paddingBytes = (512 - (size % 512)) % 512; //How long the padding to 512 bytes needs to be
			//Simply ignore the padding
			in.ignore(paddingBytes);
			//----Remove the else if and else branches if you want to handle normal files only---
		} else if (currentFileHeader.typeflag == '5') { //A directory
			//Currently long directory names are not handled correctly
			//cout << "Found directory '" << filename << "'\n";
		} else if(currentFileHeader.typeflag == 'L') {
			nextEntryHasLongName = true;
		} else {
			//Neither normal file nor directory (symlink etc.) -- currently ignored silently
			cout << "Found unhandled TAR Entry type " << currentFileHeader.typeflag << "\n";
		}
	}
}

string TarReader::GetFile(const std::string &path) const{
	return files.at(path);
}

vector<string> TarReader::GetAllPaths() const{
	vector<string> paths;
	for(const auto& pair : files){
		paths.push_back(pair.first);
	}
	return paths;
}
