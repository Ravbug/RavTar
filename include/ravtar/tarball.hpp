/*
 * tarball.h
 *
 *  Created on: Jul 28, 2010
 *      Author: Pierre Lindenbaum PhD
 *              plindenbaum@yahoo.fr
 *              http://plindenbaum.blogspot.com
 *
 * Modified on: Sep 22, 2020
 *		Author: Ravbug (https://www.ravbug.com)
 */

#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>

namespace Tar{

/**
 *  A Tar Archive
 */
class TarWriter
{
private:
	bool _finished;
    protected:
	std::ostream& out;
	void _init(void* header);
	void _checksum(void* header);
	void _size(void* header,unsigned long fileSize);
	void _filename(void* header,const char* filename);
	void _endRecord(std::size_t len);
public:
	TarWriter(std::ostream& out);
	virtual ~TarWriter();
	/** writes 2 empty blocks. Should be always called before closing the Tar file */
	void finish();
	void put(const char* filename,const std::string& s);
	void put(const char* filename,const char* content);
	void put(const char* filename,const char* content,std::size_t len);
	void putFile(const char* filename,const char* nameInArchive);
};

class TarReader{
public:
	/**
	 * Read a Tarball file
	 */
	TarReader(std::istream&);
	
	/**
	 Get a file's contents by path
	 */
	std::string GetFile(const std::string& path) const;
	
	/**
	 * Get all the paths in this Tarball
	 */
	std::vector<std::string> GetAllPaths() const;
private:
	std::unordered_map<std::string, std::string> files;
};

}
