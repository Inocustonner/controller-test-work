#include "MD5.h"
#include <Encode.hpp>

#include <string_view>
#include <cassert>
#include <Windows.h>
// https://stackoverflow.com/questions/11131353/regopenkeyex-reggetvalue-return-error-file-not-found-on-keys-that-exist
const std::wstring key_path = L"SOFTWARE\\Controller";
const std::wstring value_name = L"pwd";

std::wstring comp_name()
{
	wchar_t name[MAX_COMPUTERNAME_LENGTH + 1] = {};
	DWORD dd = std::size_t(name);
	GetComputerNameW(reinterpret_cast<wchar_t*>(name), &dd);
	return name;
}


inline
byte semibyte(std::wstring_view wstr, int i)
{
	constexpr int bits_per_byte = 8;
	return wstr[(i / 2) % std::size(wstr)] >> ((i % 2) * bits_per_byte) & 0xff;
}


inline
void xors (std::string& str, const std::wstring_view opr)
{
	constexpr int mult = sizeof(opr[0]) / sizeof(byte);
	const std::wstring salt = comp_name();
	for (int i = 0; i < std::size(str); ++i)
	{
		str[i] = str[i] ^ (semibyte(salt,i) ^ semibyte(opr, i));
	}
}

void dencode(std::string& str)
{
	static const char *pwd =  "c9c99bb56be6e9831fa19244e40fe56750c95928681f6df44e3bbaf08ecd53fc6b86b273ff34fce19d6b804eff5a3f5747ada4eaa22f1d49c01e52ddb7875b4b\0";
	static const wchar_t *pwd_w = reinterpret_cast<const wchar_t*>(pwd);
	xors(str, pwd_w);
}

// only works for caret in the beggining
inline
size_t fsize(FILE* fp)
{
	assert(ftell(fp) == 0);

	fseek(fp, 0, SEEK_END);
	size_t size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	return size;
}

// fp file ptr with write attr
void fencode(FILE* fp, std::string& src)
{
	assert(fp);

	std::string fin = MD5(src).hexdigest() + '\n';

	dencode(src);
	fin += src;
	size_t bw = fwrite(fin.c_str(), 1, std::size(fin), fp);
	if (bw != std::size(fin))
		throw std::runtime_error("Failed to write to file");
}


// fp file ptr with read attr
void fdecode(std::string& dst, FILE* fp)
{
	assert(fp);
	constexpr size_t md5len = 32;
	const size_t size = fsize(fp) - md5len - 1;
	
	std::string hash;
	hash.resize(md5len + 1);
	size_t rs = fread(hash.data(), 1, md5len + 1, fp);
	if (rs != md5len + 1)
		throw std::runtime_error("Failed to read hash");

	hash.pop_back();
	
	dst.resize(size);
	rs = fread(dst.data(), 1, size, fp);
	assert(rs == size);
	
	dencode(dst);
	if (MD5(dst).hexdigest() != hash)
		throw std::runtime_error("Hash is invalid");
}