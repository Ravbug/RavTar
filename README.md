# RavTar

This is a simple C++11 library for reading and writing tar archives. It combines code by Pierre Lindenbaum and Uli Köhler into a single library. This library does not provide compression support. 

## Usage
Reading an archive:
```cpp
#include <ravtar/tarball.hpp>
#include <fstream>

//create a reader
std::ifstream input("input.tar"); 
Tar::TarReader reader(input);

//read a file out of the archive
const std::string& contents = Tar.GetFile("included_file.txt");
```
Writing an archive:
```cpp
#include <ravtar/tarball.hpp>
#include <fstream>

// Create a tar
std::ofstream output("output.tar");
Tar::TarWriter writer(output);

// add some files
writer.putFile("path/to/file.txt","name_in_archive.txt");
writer.put("file contents here","filename.txt");

// finalize and close
writer.finish();
output.close();

```

## Compiliation
Simply use CMake:
```cmake
add_subdirectory("ravtar")
target_link_libraries("your-target" PRIVATE "ravtar")
```

## Issues
Please submit issues to the Issues section on this repository. Suggestions are welcome!
