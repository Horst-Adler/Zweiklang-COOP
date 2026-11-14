#pragma once
#include <windows.h>
#include <bcrypt.h>
#include <array>
#include <string>
#include <vector>
#include <stdexcept>
namespace zweiklang::extensions {
// Bounded streaming: no archive-sized allocation and no write access to input.
inline std::string FileSha256(const std::string& path){
 HANDLE file=CreateFileA(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
 if(file==INVALID_HANDLE_VALUE)throw std::runtime_error("Cannot read dependency file");
 BCRYPT_ALG_HANDLE alg=nullptr;BCRYPT_HASH_HANDLE hash=nullptr;std::string result;std::vector<unsigned char> object;
 try{
  if(BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)throw std::runtime_error("SHA256 unavailable");
  DWORD size=0,got=0;if(BCryptGetProperty(alg,BCRYPT_OBJECT_LENGTH,reinterpret_cast<PUCHAR>(&size),sizeof(size),&got,0)<0||size>1024*1024)throw std::runtime_error("SHA256 object error");
  object.resize(size);if(BCryptCreateHash(alg,&hash,object.data(),size,nullptr,0,0)<0)throw std::runtime_error("SHA256 create failed");
  std::array<unsigned char,65536> buffer{};DWORD read=0;
  for(;;){if(!ReadFile(file,buffer.data(),static_cast<DWORD>(buffer.size()),&read,nullptr))throw std::runtime_error("Dependency read failed");if(!read)break;if(BCryptHashData(hash,buffer.data(),read,0)<0)throw std::runtime_error("SHA256 update failed");}
  unsigned char digest[32];if(BCryptFinishHash(hash,digest,sizeof(digest),0)<0)throw std::runtime_error("SHA256 finish failed");
  const char* hex="0123456789ABCDEF";for(auto b:digest){result+=hex[b>>4];result+=hex[b&15];}
  BCryptDestroyHash(hash);hash=nullptr;BCryptCloseAlgorithmProvider(alg,0);alg=nullptr;CloseHandle(file);return result;
 }catch(...){if(hash)BCryptDestroyHash(hash);if(alg)BCryptCloseAlgorithmProvider(alg,0);CloseHandle(file);throw;}
}
}
