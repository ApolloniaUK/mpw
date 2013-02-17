#include "os.h"
#include "toolbox.h"
#include "mpw_time.h"

#include <cpu/defs.h>
#include <cpu/CpuModule.h>
#include <cpu/fmem.h>

#include <string>
#include <cerrno>
#include <cctype>
#include <algorithm>

#include <sys/xattr.h>
#include <sys/stat.h>
#include <sys/paths.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include <strings.h>

using ToolBox::Log;

namespace {

	using namespace OS;

	// should make this public since it's needed by mpw/*
	uint16_t errno_to_oserr(int xerrno)
	{
		switch (xerrno)
		{
			case 0: return 0;
			case EBADF: return rfNumErr;
			case EIO: return ioErr;
			case EACCES: return permErr;
			case ENOENT: return fnfErr;
			case ENOTDIR: return dirNFErr;
			case EISDIR: return notAFileErr;
			case ENOTSUP: return extFSErr;
			case EROFS: return wPrErr;

			case EEXIST: return dupFNErr;

			case EBUSY: return fBsyErr;

			case EDQUOT: return dskFulErr;
			case ENOSPC: return dskFulErr;


			default:
				return ioErr;
		}

	}


	std::string extension(const std::string &s)
	{
		std::string tmp;
		int pos;

		pos = s.find_last_of("./:");

		if (pos == s.npos) return tmp;		
		if (s[pos++] != '.') return tmp;
		if (pos >= s.length()) return tmp;

		tmp = s.substr(pos);

		std::transform(tmp.begin(), tmp.end(), tmp.begin(), 
			[](char c) { return tolower(c); }
		);

		return tmp;

	}

}

namespace OS
{


	// known text file extensions
	bool IsTextFile(const std::string &s)
	{
		std::string ext = extension(s);
		if (ext.empty()) return false;

		char c = ext[0];
		switch(c)
		{
			case 'a':
				if (ext == "aii") // assembler
					return true;
				if (ext == "asm")
					return true;
				break;

			case 'c':
				if (ext == "c")
					return true;
				break;
			
			case 'l':
				if (ext == "lst") // asm iigs listing
					return true;
				break;

			case 'm':
				if (ext == "macros")
					return true;
				break;

			case 'p':
				if (ext == "pii") // pascal
					return true;
				break;

			case 'r':
				if (ext == "rii") // rez
					return true;
				break;

			case 's':
				if (ext == "src") // asm equates
					return true;
				break;

		}

		return false;
	}

	// known binary file extensions
	bool IsBinaryFile(const std::string &s)
	{
		std::string ext = extension(s);
		if (ext.empty()) return false;

		char c = ext[0];
		switch(c)
		{
			case 'o':
				if (ext == "obj")
					return true;
				break;
		}

		return false;
	}



	uint16_t Create(uint16_t trap)
	{
		uint32_t d0;

		uint32_t parm = cpuGetAReg(0);

		Log("%04x Create(%08x)\n", trap, parm);

		//uint32_t ioCompletion = memoryReadLong(parm + 12);
		uint32_t namePtr = memoryReadLong(parm + 18);

		//uint16_t ioVRefNum = memoryReadWord(parm + 22);
		//uint8_t ioFVersNum = memoryReadByte(parm + 26);

		std::string sname = ToolBox::ReadPString(namePtr, true);

		if (!sname.length())
		{
			memoryWriteWord(bdNamErr, parm + 16);
			return bdNamErr;
		}
		Log("     Create(%s)\n", sname.c_str());

		int fd;
		fd = ::open(sname.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0666);

		if (fd < 0)
		{
			d0 = errno_to_oserr(errno);
		}
		else
		{
			::close(fd);
			d0 = 0;		
		}

		memoryWriteWord(d0, parm + 16);
		return d0;
	}

	uint16_t Delete(uint16_t trap)
	{
		uint32_t d0;

		uint32_t parm = cpuGetAReg(0);

		Log("%04x Delete(%08x)\n", trap, parm);

		//uint32_t ioCompletion = memoryReadLong(parm + 12);
		uint32_t namePtr = memoryReadLong(parm + 18);

		//uint16_t ioVRefNum = memoryReadWord(parm + 22);
		//uint8_t ioFVersNum = memoryReadByte(parm + 26);

		std::string sname = ToolBox::ReadPString(namePtr, true);

		if (!sname.length())
		{
			memoryWriteWord(bdNamErr, parm + 16);
			return bdNamErr;
		}
		Log("     Delete(%s)\n", sname.c_str());

		int ok = ::unlink(sname.c_str());
		if (ok < 0)
			d0 = errno_to_oserr(errno);
		else
			d0 = 0;

		memoryWriteWord(d0, parm + 16);
		return d0;
	}

	uint16_t GetEOF(uint16_t trap)
	{
		uint32_t d0;
		size_t size;

		uint32_t parm = cpuGetAReg(0);

		Log("%04x GetEOF(%08x)\n", trap, parm);

		//uint32_t ioCompletion = memoryReadLong(parm + 12);
		uint16_t ioRefNum = memoryReadWord(parm + 24);

		struct stat st;

		if (::fstat(ioRefNum, &st) < 0)
		{
			d0 = errno_to_oserr(errno);
			size = 0;
		}
		else
		{
			d0 = 0;
			size = st.st_size;
		}

		memoryWriteWord(d0, parm + 16);
		memoryWriteLong(size, parm + 28);

		return d0;
	}



	// return the name of the default volume.
	// this does not translate well.
	uint16_t GetVol(uint16_t trap)
	{
		uint32_t parm = cpuGetAReg(0);

		Log("%04x GetVol(%08x)\n", trap, parm);


		//uint32_t ioCompletion = memoryReadLong(parm + 12);
		uint32_t namePtr = memoryReadLong(parm + 18);

		// ioResult
		memoryWriteWord(0, parm + 16);
		// ioVRefNum
		memoryWriteWord(0, parm + 22);

		std::string tmp = "MacOS";
		ToolBox::WritePString(namePtr, tmp);

		return 0;
	}

	uint16_t GetFileInfo(uint16_t trap)
	{

		uint16_t d0;

		uint32_t parm = cpuGetAReg(0);

		Log("%04x GetFileInfo(%08x)\n", trap, parm);

		//uint32_t ioCompletion = memoryReadLong(parm + 12);
		uint32_t ioNamePtr = memoryReadLong(parm + 18);
		//uint16_t ioVRefNum = memoryReadWord(parm + 22);
		//uint8_t ioFVersNum = memoryReadByte(parm + 26);
		int16_t ioFDirIndex = memoryReadWord(parm + 28);

		if (ioFDirIndex <= 0)
		{
			// based name
			std::string sname;

			if (!ioNamePtr)
			{
				memoryWriteWord(bdNamErr, parm + 16);
				return bdNamErr;
			}

			sname = ToolBox::ReadPString(ioNamePtr, true);

			Log("     GetFileInfo(%s)\n", sname.c_str());

			// todo -- how are absolute, relative, etc paths handled...


			struct stat st;

			if (::stat(sname.c_str(), &st) < 0)
			{
				d0 = errno_to_oserr(errno);

				memoryWriteWord(d0, parm + 16);
				return d0;
			}

			// finder info
			{
				uint8_t buffer[32];
				std::memset(buffer, 0, sizeof(buffer));
				int rv;
				int xerrno;

				rv = ::getxattr(sname.c_str(), XATTR_FINDERINFO_NAME, buffer, 32, 0, 0);
				xerrno = errno;

				// override for source files.
				if (IsTextFile(sname))
				{
					std::memcpy(buffer, "TEXTMPS ", 8);
				}

				// only 16 bytes copied.
				std::memcpy(memoryPointer(parm + 32), buffer, 16);

			}


			// file reference number
			memoryWriteWord(0, parm + 24);
			// file attributes
			memoryWriteByte(0, parm + 30);
			// version (unused)
			memoryWriteByte(0, parm + 31);

			// file id
			memoryWriteLong(0, parm + 48);


			// file size
			memoryWriteWord(0, parm + 52);
			memoryWriteLong(st.st_size, parm + 54);
			memoryWriteLong(st.st_size, parm + 58);

			// create date.
			memoryWriteLong(Time::UnixToMac(st.st_birthtime), parm + 72);
			memoryWriteLong(Time::UnixToMac(st.st_mtime), parm + 76);

			// res fork...
			// do this last since it adjusts the name and the stat.

			sname.append(_PATH_RSRCFORKSPEC);
			if (::stat(sname.c_str(), &st) >= 0)
			{
				memoryWriteWord(0, parm + 62);
				memoryWriteLong(st.st_size, parm + 64);
				memoryWriteLong(st.st_size, parm + 68);
			}
			else
			{
				memoryWriteWord(0, parm + 62);
				memoryWriteLong(0, parm + 64);
				memoryWriteLong(0, parm + 68);
			}

			// no error.
			memoryWriteWord(0, parm + 16);


		}
		else
		{
			fprintf(stderr, "GetFileInfo -- ioFDirIndex not yet supported\n");
			exit(1);
		}

		// if iocompletion handler && asyn call....

		return 0;
	}

	uint16_t SetFileInfo(uint16_t trap)
	{
		std::string sname;
		uint16_t d0;


		uint32_t parm = cpuGetAReg(0);

		Log("%04x SetFileInfo(%08x)\n", trap, parm);

		//uint32_t ioCompletion = memoryReadLong(parm + 12);
		uint32_t ioNamePtr = memoryReadLong(parm + 18);
		//uint16_t ioVRefNum = memoryReadWord(parm + 22);
		//uint8_t ioFVersNum = memoryReadByte(parm + 26);
		//int16_t ioFDirIndex = memoryReadWord(parm + 28);

		// + 32 = finder data - 16 bytes.

		//uint32_t ioFlCrDat = memoryReadLong(parm + 72);
		//uint32_t ioFlMdDat = memoryReadLong(parm + 76);

		// currently, only sets finder info.

		if (!ioNamePtr)
		{
			memoryWriteWord(bdNamErr, parm + 16);
			return bdNamErr;
		}

		sname = ToolBox::ReadPString(ioNamePtr, true);
		Log("     SetFileInfo(%s)\n", sname.c_str());

		// check if the file actually exists
		{
			struct stat st;
			int ok;

			ok = ::stat(sname.c_str(), &st);
			if (ok < 0)
			{
				d0 = errno_to_oserr(errno);
				memoryWriteWord(d0, parm + 16);
				return d0; 
			}


		}

		// finder info is actually 32 bytes, so read and update the first 16.
		{
			uint8_t buffer[32];
			int ok;

			std::memset(buffer, 0, sizeof(buffer));

			ok = ::getxattr(sname.c_str(), XATTR_FINDERINFO_NAME, buffer, 32, 0, 0);

			// only 16 bytes copied.
			std::memcpy(buffer, memoryPointer(parm + 32), 16);

			ok = ::setxattr(sname.c_str(), XATTR_FINDERINFO_NAME, buffer, 32, 0, 0);	

			if (ok < 0)
			{
				d0 = errno_to_oserr(errno);
				memoryWriteWord(d0, parm + 16);
				return d0; 				
			}
		} 

		d0 = 0;
		memoryWriteWord(d0, parm + 16);
		return d0;
	}


	#pragma mark string utilities

	uint16_t CmpString(uint16_t trap)
	{

		/* 
		 * on entry:
		 * A0 Pointer to first character of first string
		 * A1 Pointer to first character of second string
		 * D0 (high) length of first string 
		 * D0 (low) length of second string
		 *
		 * on exit:
		 * D0 0 if strings equal, 1 if strings not equal.
		 *
		 */

		bool caseSens = trap & (1 << 9);
		//bool diacSens = trap & (1 << 10); // ignore for now...

		uint32_t aStr = cpuGetAReg(0);
		uint32_t bStr = cpuGetAReg(1);

		uint32_t length = cpuGetDReg(0);

		uint32_t aLen = (length >> 16);
		uint32_t bLen = (length & 0xffff);

		std::string a = ToolBox::ReadString(aStr, aLen);
		std::string b = ToolBox::ReadString(bStr, bLen);

		Log("%04x CmpString(%s, %s)\n", trap, a.c_str(), b.c_str());

		if (aLen != bLen) return 1; // different length...
		if (aStr == bStr) return 0; // same ptr...

		bool eq;
		eq = std::equal(
			a.begin(), 
			a.end(), 
			b.begin(), 
			[caseSens](char a, char b){
				if (a == b) return true;
				if (!caseSens)
				{
					a = toupper(a);
					b = toupper(b);
				}
				return a == b;
			}
		);

		return eq ? 0 : 1;
	}


}