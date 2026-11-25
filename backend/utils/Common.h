#pragma once
#include "../models/Json.h"
#include "../models/Task.h"
#include <sqlite3.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <random>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <vector>
#ifdef _WIN32
#define NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>
#include <bcrypt.h>
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"bcrypt.lib")
using Socket=SOCKET;
static constexpr Socket invalidSocket=INVALID_SOCKET;
static void closeSocket(Socket s)
{
    closesocket(s);
}
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
using Socket=int;
static constexpr Socket invalidSocket=-1;
static void closeSocket(Socket s)
{
    close(s);
}
#endif
namespace fs=std::filesystem;
static Json obj(std::initializer_list<std::pair<const std::string,Json>> v)
{
    return Json(Json::Object(v));
}
static Json err(const std::string&s)
{
    return obj(
    {
        {
            "error",s
        }
    }
    );
}
static std::string get(const Json&j,const std::string&k,const std::string&fallback="")
{
    return j.at(k).str(fallback);
}
static double number(const Json&j,const std::string&k,double fallback=0)
{
    return j.at(k).num(fallback);
}
static bool in(const std::string&s,std::initializer_list<const char*> choices)
{
    for(auto c:choices)if(s==c)return true;
    return false;
}
static std::string hex(const unsigned char* b,size_t n)
{
    const char*d="0123456789abcdef";
    std::string s;
    for(size_t i=0;i<n;i++)
    {
        s+=d[b[i]>>4];
        s+=d[b[i]&15];
    }
    return s;
}
static std::vector<unsigned char> unhex(const std::string&s)
{
    if(s.size()%2)throw std::runtime_error("Bad hash");
    std::vector<unsigned char> b;
    for(size_t i=0;i<s.size();i+=2)
    {
        unsigned v=0;
        for(int j=0;j<2;j++)
        {
            char c=s[i+j];
            v=v*16+(c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:c>='A'&&c<='F'?c-'A'+10:throw std::runtime_error("Bad hash"));
        }
        b.push_back((unsigned char)v);
    }
    return b;
}
static std::vector<unsigned char> randomBytes(size_t n)
{
    std::vector<unsigned char> v(n);
#ifdef _WIN32
    if(BCryptGenRandom(nullptr,v.data(),(ULONG)v.size(),BCRYPT_USE_SYSTEM_PREFERRED_RNG)!=0)throw std::runtime_error("Random generation failed");
#else
    if(RAND_bytes(v.data(),(int)v.size())!=1)throw std::runtime_error("Random generation failed");
#endif
    return v;
}
static std::string passwordHash(const std::string&pass,const std::string&saltHex)
{
    auto salt=unhex(saltHex);
    std::array<unsigned char,32> out
    {
    };
#ifdef _WIN32
    BCRYPT_ALG_HANDLE h=nullptr;
    if(BCryptOpenAlgorithmProvider(&h,BCRYPT_SHA256_ALGORITHM,nullptr,BCRYPT_ALG_HANDLE_HMAC_FLAG)!=0)throw std::runtime_error("PBKDF2 provider failed");
    auto rc=BCryptDeriveKeyPBKDF2(h,(PUCHAR)pass.data(),(ULONG)pass.size(),salt.data(),(ULONG)salt.size(),210000,out.data(),(ULONG)out.size(),0);
    BCryptCloseAlgorithmProvider(h,0);
    if(rc!=0)throw std::runtime_error("PBKDF2 failed");
#else
    if(PKCS5_PBKDF2_HMAC(pass.c_str(),(int)pass.size(),salt.data(),(int)salt.size(),210000,EVP_sha256(),(int)out.size(),out.data())!=1)throw std::runtime_error("PBKDF2 failed");
#endif
    return hex(out.data(),out.size());
}
static bool secureEqual(const std::string&a,const std::string&b)
{
    unsigned char diff=(unsigned char)(a.size()^b.size());
    size_t n=std::max(a.size(),b.size());
    for(size_t i=0;i<n;i++)diff|=(unsigned char)((i<a.size()?a[i]:0)^(i<b.size()?b[i]:0));
    return diff==0;
}
static std::string today()
{
    auto now=std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm t
    {
    };
#ifdef _WIN32
    localtime_s(&t,&now);
#else
    localtime_r(&now,&t);
#endif
    char buf[20];
    std::strftime(buf,sizeof(buf),"%Y-%m-%d",&t);
    return buf;
}
static bool validDate(const std::string&s)
{
    if(s.size()!=10||s[4]!='-'||s[7]!='-')return false;
    for(int i=0;i<10;i++)if(i!=4&&i!=7&&!std::isdigit((unsigned char)s[i]))return false;
    int y=std::stoi(s.substr(0,4)),m=std::stoi(s.substr(5,2)),d=std::stoi(s.substr(8,2));
    if(y<2000||y>2100||m<1||m>12||d<1)return false;
    int ds[]=
    {
        31,28+((y%4==0&&y%100!=0)||y%400==0),31,30,31,30,31,31,30,31,30,31
    };
    return d<=ds[m-1];
}
static std::tm dateTm(const std::string&s)
{
    std::tm t
    {
    };
    t.tm_year=std::stoi(s.substr(0,4))-1900;
    t.tm_mon=std::stoi(s.substr(5,2))-1;
    t.tm_mday=std::stoi(s.substr(8,2));
    t.tm_hour=12;
    t.tm_isdst=-1;
    return t;
}
static std::string offsetDate(const std::string&s,int n)
{
    auto t=dateTm(s);
    t.tm_mday+=n;
    std::mktime(&t);
    char b[16];
    std::strftime(b,sizeof(b),"%Y-%m-%d",&t);
    return b;
}
static int weekday(const std::string&s)
{
    auto t=dateTm(s);
    std::mktime(&t);
    return (t.tm_wday+6)%7;
}
