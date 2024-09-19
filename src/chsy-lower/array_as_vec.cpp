#include <fmt/format.h>
#include <xcml_type.hpp>
#include <xcml_utils.hpp>
#include "chsy-lower.hpp"

namespace u = xcml::utils;

static char const* TYPEDEFS();
static char const* VEC_FUNCS();

xcml::xcml_program_node_ptr array_as_vec(xcml::xcml_program_node_ptr prg) {
    auto code = u::new_code();
    code->value = TYPEDEFS();
    code->value += VEC_FUNCS();
    prg->preamble.push_back(code);

    for (auto& type : prg->type_table) {
        if (auto bt = xcml::basic_type::dyncast(type); bt && bt->veclen > 0) {
            char sig;

            if (bt->name == "char") {
                sig = 'c';
            } else if (bt->name == "unsigned_char") {
                sig = 'h';
            } else if (bt->name == "short") {
                sig = 's';
            } else if (bt->name == "unsigned_short") {
                sig = 't';
            } else if (bt->name == "int") {
                sig = 'i';
            } else if (bt->name == "unsigned") {
                sig = 'j';
            } else if (bt->name == "long") {
                sig = 'l';
            } else if (bt->name == "unsigned_long") {
                sig = 'm';
            } else if (bt->name == "longlong") {
                sig = 'x';
            } else if (bt->name == "unsigned_longlong") {
                sig = 'y';
            } else if (bt->name == "float") {
                sig = 'f';
            } else if (bt->name == "double") {
                sig = 'd';
            } else {
                std::abort();
            }

            bt->name = fmt::format("_s__v{}{}", bt->veclen, sig);
            bt->is_builtin = true;
            bt->veclen = 0;
        }
    }

    return prg;
}

char const* TYPEDEFS() {
    return R"(
#if defined(__cpluslpus)
#  if defined(__has_attribute) && __has_attribute(maybe_unused)
#    define _MaybeUnused [[maybe_unused]]
#  elif __cplusplus >= 201703L
#    define _MaybeUnused [[maybe_unused]]
#  endif
#endif

#if defined(__cpluslpus)
#  if __cplusplus >= 201103L
#    define _AlignAs(_T, _N) alignas(sizeof(_T) * _N)
#  endif
#endif

#if defined(__GNUC__) || defined(__NVCC__) || defined(__HIPCC__)
#  define _Aligned(_T, _N) __attribute__((aligned(sizeof(_T) * (_N == 3 ? 4 : _N))))
#  ifndef _MaybeUnused
#    define _MaybeUnused __attribute__((unused))
#  endif
#endif

#if defined(_AlignAs) && defined(_Aligned)
#  undef _Aligned
#  define _Aligned(_T, _N)
#elif defined(_Aligned)
#  define _AlignAs(_T, _N)
#else
#  error This compiler does not support the aligned attribute.
#endif

#define _VectorType(_T, _N) struct _AlignAs(_T, _N) { _T __v[_N]; } _Aligned(_T, _N)

#ifndef _MaybeUnused
#  define _MaybeUnused
#endif

#if !defined(__HIPCC_RTC__) && !defined(__NVCC__) && !defined(__device__)
#  define __device__
#  define __device__defined
#endif

typedef _VectorType(char,1) _s__v1c;
typedef _VectorType(char,2) _s__v2c;
typedef _VectorType(char,3) _s__v3c;
typedef _VectorType(char,4) _s__v4c;
typedef _VectorType(char,8) _s__v8c;
typedef _VectorType(char,16) _s__v16c;
typedef _VectorType(unsigned char,1) _s__v1h;
typedef _VectorType(unsigned char,2) _s__v2h;
typedef _VectorType(unsigned char,3) _s__v3h;
typedef _VectorType(unsigned char,4) _s__v4h;
typedef _VectorType(unsigned char,8) _s__v8h;
typedef _VectorType(unsigned char,16) _s__v16h;
typedef _VectorType(short,1) _s__v1s;
typedef _VectorType(short,2) _s__v2s;
typedef _VectorType(short,3) _s__v3s;
typedef _VectorType(short,4) _s__v4s;
typedef _VectorType(short,8) _s__v8s;
typedef _VectorType(short,16) _s__v16s;
typedef _VectorType(unsigned short,1) _s__v1t;
typedef _VectorType(unsigned short,2) _s__v2t;
typedef _VectorType(unsigned short,3) _s__v3t;
typedef _VectorType(unsigned short,4) _s__v4t;
typedef _VectorType(unsigned short,8) _s__v8t;
typedef _VectorType(unsigned short,16) _s__v16t;
typedef _VectorType(int,1) _s__v1i;
typedef _VectorType(int,2) _s__v2i;
typedef _VectorType(int,3) _s__v3i;
typedef _VectorType(int,4) _s__v4i;
typedef _VectorType(int,8) _s__v8i;
typedef _VectorType(int,16) _s__v16i;
typedef _VectorType(unsigned int,1) _s__v1j;
typedef _VectorType(unsigned int,2) _s__v2j;
typedef _VectorType(unsigned int,3) _s__v3j;
typedef _VectorType(unsigned int,4) _s__v4j;
typedef _VectorType(unsigned int,8) _s__v8j;
typedef _VectorType(unsigned int,16) _s__v16j;
typedef _VectorType(long,1) _s__v1l;
typedef _VectorType(long,2) _s__v2l;
typedef _VectorType(long,3) _s__v3l;
typedef _VectorType(long,4) _s__v4l;
typedef _VectorType(long,8) _s__v8l;
typedef _VectorType(long,16) _s__v16l;
typedef _VectorType(unsigned long,1) _s__v1m;
typedef _VectorType(unsigned long,2) _s__v2m;
typedef _VectorType(unsigned long,3) _s__v3m;
typedef _VectorType(unsigned long,4) _s__v4m;
typedef _VectorType(unsigned long,8) _s__v8m;
typedef _VectorType(unsigned long,16) _s__v16m;
typedef _VectorType(long long,1) _s__v1x;
typedef _VectorType(long long,2) _s__v2x;
typedef _VectorType(long long,3) _s__v3x;
typedef _VectorType(long long,4) _s__v4x;
typedef _VectorType(long long,8) _s__v8x;
typedef _VectorType(long long,16) _s__v16x;
typedef _VectorType(unsigned long long,1) _s__v1y;
typedef _VectorType(unsigned long long,2) _s__v2y;
typedef _VectorType(unsigned long long,3) _s__v3y;
typedef _VectorType(unsigned long long,4) _s__v4y;
typedef _VectorType(unsigned long long,8) _s__v8y;
typedef _VectorType(unsigned long long,16) _s__v16y;
typedef _VectorType(float,1) _s__v1f;
typedef _VectorType(float,2) _s__v2f;
typedef _VectorType(float,3) _s__v3f;
typedef _VectorType(float,4) _s__v4f;
typedef _VectorType(float,8) _s__v8f;
typedef _VectorType(float,16) _s__v16f;
typedef _VectorType(double,1) _s__v1d;
typedef _VectorType(double,2) _s__v2d;
typedef _VectorType(double,3) _s__v3d;
typedef _VectorType(double,4) _s__v4d;
typedef _VectorType(double,8) _s__v8d;
typedef _VectorType(double,16) _s__v16d;
#undef _VectorType
)";
}

char const* VEC_FUNCS() {
    return R"(
static inline __device__ _MaybeUnused _s__v1c __charm_sycl_vec_v1c(char a) { return (_s__v1c){a}; }
static inline __device__ _MaybeUnused _s__v2c __charm_sycl_vec_v2c(char a,char b) { return (_s__v2c){a,b}; }
static inline __device__ _MaybeUnused _s__v3c __charm_sycl_vec_v3c(char a,char b,char c) { return (_s__v3c){a,b,c}; }
static inline __device__ _MaybeUnused _s__v4c __charm_sycl_vec_v4c(char a,char b,char c,char d) { return (_s__v4c){a,b,c,d}; }
static inline __device__ _MaybeUnused _s__v8c __charm_sycl_vec_v8c(char a,char b,char c,char d,char e,char f,char g,char h) { return (_s__v8c){a,b,c,d,e,f,g,h}; }
static inline __device__ _MaybeUnused _s__v16c __charm_sycl_vec_v16c(char a,char b,char c,char d,char e,char f,char g,char h,char i,char j,char k,char l,char m,char n,char o,char p) { return (_s__v16c){a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p}; }
static inline __device__ _MaybeUnused _s__v1h __charm_sycl_vec_v1h(unsigned char a) { return (_s__v1h){a}; }
static inline __device__ _MaybeUnused _s__v2h __charm_sycl_vec_v2h(unsigned char a,unsigned char b) { return (_s__v2h){a,b}; }
static inline __device__ _MaybeUnused _s__v3h __charm_sycl_vec_v3h(unsigned char a,unsigned char b,unsigned char c) { return (_s__v3h){a,b,c}; }
static inline __device__ _MaybeUnused _s__v4h __charm_sycl_vec_v4h(unsigned char a,unsigned char b,unsigned char c,unsigned char d) { return (_s__v4h){a,b,c,d}; }
static inline __device__ _MaybeUnused _s__v8h __charm_sycl_vec_v8h(unsigned char a,unsigned char b,unsigned char c,unsigned char d,unsigned char e,unsigned char f,unsigned char g,unsigned char h) { return (_s__v8h){a,b,c,d,e,f,g,h}; }
static inline __device__ _MaybeUnused _s__v16h __charm_sycl_vec_v16h(unsigned char a,unsigned char b,unsigned char c,unsigned char d,unsigned char e,unsigned char f,unsigned char g,unsigned char h,unsigned char i,unsigned char j,unsigned char k,unsigned char l,unsigned char m,unsigned char n,unsigned char o,unsigned char p) { return (_s__v16h){a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p}; }
static inline __device__ _MaybeUnused _s__v1s __charm_sycl_vec_v1s(short a) { return (_s__v1s){a}; }
static inline __device__ _MaybeUnused _s__v2s __charm_sycl_vec_v2s(short a,short b) { return (_s__v2s){a,b}; }
static inline __device__ _MaybeUnused _s__v3s __charm_sycl_vec_v3s(short a,short b,short c) { return (_s__v3s){a,b,c}; }
static inline __device__ _MaybeUnused _s__v4s __charm_sycl_vec_v4s(short a,short b,short c,short d) { return (_s__v4s){a,b,c,d}; }
static inline __device__ _MaybeUnused _s__v8s __charm_sycl_vec_v8s(short a,short b,short c,short d,short e,short f,short g,short h) { return (_s__v8s){a,b,c,d,e,f,g,h}; }
static inline __device__ _MaybeUnused _s__v16s __charm_sycl_vec_v16s(short a,short b,short c,short d,short e,short f,short g,short h,short i,short j,short k,short l,short m,short n,short o,short p) { return (_s__v16s){a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p}; }
static inline __device__ _MaybeUnused _s__v1t __charm_sycl_vec_v1t(unsigned short a) { return (_s__v1t){a}; }
static inline __device__ _MaybeUnused _s__v2t __charm_sycl_vec_v2t(unsigned short a,unsigned short b) { return (_s__v2t){a,b}; }
static inline __device__ _MaybeUnused _s__v3t __charm_sycl_vec_v3t(unsigned short a,unsigned short b,unsigned short c) { return (_s__v3t){a,b,c}; }
static inline __device__ _MaybeUnused _s__v4t __charm_sycl_vec_v4t(unsigned short a,unsigned short b,unsigned short c,unsigned short d) { return (_s__v4t){a,b,c,d}; }
static inline __device__ _MaybeUnused _s__v8t __charm_sycl_vec_v8t(unsigned short a,unsigned short b,unsigned short c,unsigned short d,unsigned short e,unsigned short f,unsigned short g,unsigned short h) { return (_s__v8t){a,b,c,d,e,f,g,h}; }
static inline __device__ _MaybeUnused _s__v16t __charm_sycl_vec_v16t(unsigned short a,unsigned short b,unsigned short c,unsigned short d,unsigned short e,unsigned short f,unsigned short g,unsigned short h,unsigned short i,unsigned short j,unsigned short k,unsigned short l,unsigned short m,unsigned short n,unsigned short o,unsigned short p) { return (_s__v16t){a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p}; }
static inline __device__ _MaybeUnused _s__v1i __charm_sycl_vec_v1i(int a) { return (_s__v1i){a}; }
static inline __device__ _MaybeUnused _s__v2i __charm_sycl_vec_v2i(int a,int b) { return (_s__v2i){a,b}; }
static inline __device__ _MaybeUnused _s__v3i __charm_sycl_vec_v3i(int a,int b,int c) { return (_s__v3i){a,b,c}; }
static inline __device__ _MaybeUnused _s__v4i __charm_sycl_vec_v4i(int a,int b,int c,int d) { return (_s__v4i){a,b,c,d}; }
static inline __device__ _MaybeUnused _s__v8i __charm_sycl_vec_v8i(int a,int b,int c,int d,int e,int f,int g,int h) { return (_s__v8i){a,b,c,d,e,f,g,h}; }
static inline __device__ _MaybeUnused _s__v16i __charm_sycl_vec_v16i(int a,int b,int c,int d,int e,int f,int g,int h,int i,int j,int k,int l,int m,int n,int o,int p) { return (_s__v16i){a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p}; }
static inline __device__ _MaybeUnused _s__v1j __charm_sycl_vec_v1j(unsigned int a) { return (_s__v1j){a}; }
static inline __device__ _MaybeUnused _s__v2j __charm_sycl_vec_v2j(unsigned int a,unsigned int b) { return (_s__v2j){a,b}; }
static inline __device__ _MaybeUnused _s__v3j __charm_sycl_vec_v3j(unsigned int a,unsigned int b,unsigned int c) { return (_s__v3j){a,b,c}; }
static inline __device__ _MaybeUnused _s__v4j __charm_sycl_vec_v4j(unsigned int a,unsigned int b,unsigned int c,unsigned int d) { return (_s__v4j){a,b,c,d}; }
static inline __device__ _MaybeUnused _s__v8j __charm_sycl_vec_v8j(unsigned int a,unsigned int b,unsigned int c,unsigned int d,unsigned int e,unsigned int f,unsigned int g,unsigned int h) { return (_s__v8j){a,b,c,d,e,f,g,h}; }
static inline __device__ _MaybeUnused _s__v16j __charm_sycl_vec_v16j(unsigned int a,unsigned int b,unsigned int c,unsigned int d,unsigned int e,unsigned int f,unsigned int g,unsigned int h,unsigned int i,unsigned int j,unsigned int k,unsigned int l,unsigned int m,unsigned int n,unsigned int o,unsigned int p) { return (_s__v16j){a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p}; }
static inline __device__ _MaybeUnused _s__v1l __charm_sycl_vec_v1l(long a) { return (_s__v1l){a}; }
static inline __device__ _MaybeUnused _s__v2l __charm_sycl_vec_v2l(long a,long b) { return (_s__v2l){a,b}; }
static inline __device__ _MaybeUnused _s__v3l __charm_sycl_vec_v3l(long a,long b,long c) { return (_s__v3l){a,b,c}; }
static inline __device__ _MaybeUnused _s__v4l __charm_sycl_vec_v4l(long a,long b,long c,long d) { return (_s__v4l){a,b,c,d}; }
static inline __device__ _MaybeUnused _s__v8l __charm_sycl_vec_v8l(long a,long b,long c,long d,long e,long f,long g,long h) { return (_s__v8l){a,b,c,d,e,f,g,h}; }
static inline __device__ _MaybeUnused _s__v16l __charm_sycl_vec_v16l(long a,long b,long c,long d,long e,long f,long g,long h,long i,long j,long k,long l,long m,long n,long o,long p) { return (_s__v16l){a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p}; }
static inline __device__ _MaybeUnused _s__v1m __charm_sycl_vec_v1m(unsigned long a) { return (_s__v1m){a}; }
static inline __device__ _MaybeUnused _s__v2m __charm_sycl_vec_v2m(unsigned long a,unsigned long b) { return (_s__v2m){a,b}; }
static inline __device__ _MaybeUnused _s__v3m __charm_sycl_vec_v3m(unsigned long a,unsigned long b,unsigned long c) { return (_s__v3m){a,b,c}; }
static inline __device__ _MaybeUnused _s__v4m __charm_sycl_vec_v4m(unsigned long a,unsigned long b,unsigned long c,unsigned long d) { return (_s__v4m){a,b,c,d}; }
static inline __device__ _MaybeUnused _s__v8m __charm_sycl_vec_v8m(unsigned long a,unsigned long b,unsigned long c,unsigned long d,unsigned long e,unsigned long f,unsigned long g,unsigned long h) { return (_s__v8m){a,b,c,d,e,f,g,h}; }
static inline __device__ _MaybeUnused _s__v16m __charm_sycl_vec_v16m(unsigned long a,unsigned long b,unsigned long c,unsigned long d,unsigned long e,unsigned long f,unsigned long g,unsigned long h,unsigned long i,unsigned long j,unsigned long k,unsigned long l,unsigned long m,unsigned long n,unsigned long o,unsigned long p) { return (_s__v16m){a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p}; }
static inline __device__ _MaybeUnused _s__v1x __charm_sycl_vec_v1x(long long a) { return (_s__v1x){a}; }
static inline __device__ _MaybeUnused _s__v2x __charm_sycl_vec_v2x(long long a,long long b) { return (_s__v2x){a,b}; }
static inline __device__ _MaybeUnused _s__v3x __charm_sycl_vec_v3x(long long a,long long b,long long c) { return (_s__v3x){a,b,c}; }
static inline __device__ _MaybeUnused _s__v4x __charm_sycl_vec_v4x(long long a,long long b,long long c,long long d) { return (_s__v4x){a,b,c,d}; }
static inline __device__ _MaybeUnused _s__v8x __charm_sycl_vec_v8x(long long a,long long b,long long c,long long d,long long e,long long f,long long g,long long h) { return (_s__v8x){a,b,c,d,e,f,g,h}; }
static inline __device__ _MaybeUnused _s__v16x __charm_sycl_vec_v16x(long long a,long long b,long long c,long long d,long long e,long long f,long long g,long long h,long long i,long long j,long long k,long long l,long long m,long long n,long long o,long long p) { return (_s__v16x){a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p}; }
static inline __device__ _MaybeUnused _s__v1y __charm_sycl_vec_v1y(unsigned long long a) { return (_s__v1y){a}; }
static inline __device__ _MaybeUnused _s__v2y __charm_sycl_vec_v2y(unsigned long long a,unsigned long long b) { return (_s__v2y){a,b}; }
static inline __device__ _MaybeUnused _s__v3y __charm_sycl_vec_v3y(unsigned long long a,unsigned long long b,unsigned long long c) { return (_s__v3y){a,b,c}; }
static inline __device__ _MaybeUnused _s__v4y __charm_sycl_vec_v4y(unsigned long long a,unsigned long long b,unsigned long long c,unsigned long long d) { return (_s__v4y){a,b,c,d}; }
static inline __device__ _MaybeUnused _s__v8y __charm_sycl_vec_v8y(unsigned long long a,unsigned long long b,unsigned long long c,unsigned long long d,unsigned long long e,unsigned long long f,unsigned long long g,unsigned long long h) { return (_s__v8y){a,b,c,d,e,f,g,h}; }
static inline __device__ _MaybeUnused _s__v16y __charm_sycl_vec_v16y(unsigned long long a,unsigned long long b,unsigned long long c,unsigned long long d,unsigned long long e,unsigned long long f,unsigned long long g,unsigned long long h,unsigned long long i,unsigned long long j,unsigned long long k,unsigned long long l,unsigned long long m,unsigned long long n,unsigned long long o,unsigned long long p) { return (_s__v16y){a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p}; }
static inline __device__ _MaybeUnused _s__v1f __charm_sycl_vec_v1f(float a) { return (_s__v1f){a}; }
static inline __device__ _MaybeUnused _s__v2f __charm_sycl_vec_v2f(float a,float b) { return (_s__v2f){a,b}; }
static inline __device__ _MaybeUnused _s__v3f __charm_sycl_vec_v3f(float a,float b,float c) { return (_s__v3f){a,b,c}; }
static inline __device__ _MaybeUnused _s__v4f __charm_sycl_vec_v4f(float a,float b,float c,float d) { return (_s__v4f){a,b,c,d}; }
static inline __device__ _MaybeUnused _s__v8f __charm_sycl_vec_v8f(float a,float b,float c,float d,float e,float f,float g,float h) { return (_s__v8f){a,b,c,d,e,f,g,h}; }
static inline __device__ _MaybeUnused _s__v16f __charm_sycl_vec_v16f(float a,float b,float c,float d,float e,float f,float g,float h,float i,float j,float k,float l,float m,float n,float o,float p) { return (_s__v16f){a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p}; }
static inline __device__ _MaybeUnused _s__v1d __charm_sycl_vec_v1d(double a) { return (_s__v1d){a}; }
static inline __device__ _MaybeUnused _s__v2d __charm_sycl_vec_v2d(double a,double b) { return (_s__v2d){a,b}; }
static inline __device__ _MaybeUnused _s__v3d __charm_sycl_vec_v3d(double a,double b,double c) { return (_s__v3d){a,b,c}; }
static inline __device__ _MaybeUnused _s__v4d __charm_sycl_vec_v4d(double a,double b,double c,double d) { return (_s__v4d){a,b,c,d}; }
static inline __device__ _MaybeUnused _s__v8d __charm_sycl_vec_v8d(double a,double b,double c,double d,double e,double f,double g,double h) { return (_s__v8d){a,b,c,d,e,f,g,h}; }
static inline __device__ _MaybeUnused _s__v16d __charm_sycl_vec_v16d(double a,double b,double c,double d,double e,double f,double g,double h,double i,double j,double k,double l,double m,double n,double o,double p) { return (_s__v16d){a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p}; }

static inline __device__ _MaybeUnused _s__v1c __charm_sycl_vec_splat_v1c(char x){ return __charm_sycl_vec_v1c(x); }
static inline __device__ _MaybeUnused _s__v2c __charm_sycl_vec_splat_v2c(char x){ return __charm_sycl_vec_v2c(x,x); }
static inline __device__ _MaybeUnused _s__v3c __charm_sycl_vec_splat_v3c(char x){ return __charm_sycl_vec_v3c(x,x,x); }
static inline __device__ _MaybeUnused _s__v4c __charm_sycl_vec_splat_v4c(char x){ return __charm_sycl_vec_v4c(x,x,x,x); }
static inline __device__ _MaybeUnused _s__v8c __charm_sycl_vec_splat_v8c(char x){ return __charm_sycl_vec_v8c(x,x,x,x,x,x,x,x); }
static inline __device__ _MaybeUnused _s__v16c __charm_sycl_vec_splat_v16c(char x){ return __charm_sycl_vec_v16c(x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x); }
static inline __device__ _MaybeUnused _s__v1h __charm_sycl_vec_splat_v1h(unsigned char x){ return __charm_sycl_vec_v1h(x); }
static inline __device__ _MaybeUnused _s__v2h __charm_sycl_vec_splat_v2h(unsigned char x){ return __charm_sycl_vec_v2h(x,x); }
static inline __device__ _MaybeUnused _s__v3h __charm_sycl_vec_splat_v3h(unsigned char x){ return __charm_sycl_vec_v3h(x,x,x); }
static inline __device__ _MaybeUnused _s__v4h __charm_sycl_vec_splat_v4h(unsigned char x){ return __charm_sycl_vec_v4h(x,x,x,x); }
static inline __device__ _MaybeUnused _s__v8h __charm_sycl_vec_splat_v8h(unsigned char x){ return __charm_sycl_vec_v8h(x,x,x,x,x,x,x,x); }
static inline __device__ _MaybeUnused _s__v16h __charm_sycl_vec_splat_v16h(unsigned char x){ return __charm_sycl_vec_v16h(x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x); }
static inline __device__ _MaybeUnused _s__v1s __charm_sycl_vec_splat_v1s(short x){ return __charm_sycl_vec_v1s(x); }
static inline __device__ _MaybeUnused _s__v2s __charm_sycl_vec_splat_v2s(short x){ return __charm_sycl_vec_v2s(x,x); }
static inline __device__ _MaybeUnused _s__v3s __charm_sycl_vec_splat_v3s(short x){ return __charm_sycl_vec_v3s(x,x,x); }
static inline __device__ _MaybeUnused _s__v4s __charm_sycl_vec_splat_v4s(short x){ return __charm_sycl_vec_v4s(x,x,x,x); }
static inline __device__ _MaybeUnused _s__v8s __charm_sycl_vec_splat_v8s(short x){ return __charm_sycl_vec_v8s(x,x,x,x,x,x,x,x); }
static inline __device__ _MaybeUnused _s__v16s __charm_sycl_vec_splat_v16s(short x){ return __charm_sycl_vec_v16s(x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x); }
static inline __device__ _MaybeUnused _s__v1t __charm_sycl_vec_splat_v1t(unsigned short x){ return __charm_sycl_vec_v1t(x); }
static inline __device__ _MaybeUnused _s__v2t __charm_sycl_vec_splat_v2t(unsigned short x){ return __charm_sycl_vec_v2t(x,x); }
static inline __device__ _MaybeUnused _s__v3t __charm_sycl_vec_splat_v3t(unsigned short x){ return __charm_sycl_vec_v3t(x,x,x); }
static inline __device__ _MaybeUnused _s__v4t __charm_sycl_vec_splat_v4t(unsigned short x){ return __charm_sycl_vec_v4t(x,x,x,x); }
static inline __device__ _MaybeUnused _s__v8t __charm_sycl_vec_splat_v8t(unsigned short x){ return __charm_sycl_vec_v8t(x,x,x,x,x,x,x,x); }
static inline __device__ _MaybeUnused _s__v16t __charm_sycl_vec_splat_v16t(unsigned short x){ return __charm_sycl_vec_v16t(x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x); }
static inline __device__ _MaybeUnused _s__v1i __charm_sycl_vec_splat_v1i(int x){ return __charm_sycl_vec_v1i(x); }
static inline __device__ _MaybeUnused _s__v2i __charm_sycl_vec_splat_v2i(int x){ return __charm_sycl_vec_v2i(x,x); }
static inline __device__ _MaybeUnused _s__v3i __charm_sycl_vec_splat_v3i(int x){ return __charm_sycl_vec_v3i(x,x,x); }
static inline __device__ _MaybeUnused _s__v4i __charm_sycl_vec_splat_v4i(int x){ return __charm_sycl_vec_v4i(x,x,x,x); }
static inline __device__ _MaybeUnused _s__v8i __charm_sycl_vec_splat_v8i(int x){ return __charm_sycl_vec_v8i(x,x,x,x,x,x,x,x); }
static inline __device__ _MaybeUnused _s__v16i __charm_sycl_vec_splat_v16i(int x){ return __charm_sycl_vec_v16i(x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x); }
static inline __device__ _MaybeUnused _s__v1j __charm_sycl_vec_splat_v1j(unsigned int x){ return __charm_sycl_vec_v1j(x); }
static inline __device__ _MaybeUnused _s__v2j __charm_sycl_vec_splat_v2j(unsigned int x){ return __charm_sycl_vec_v2j(x,x); }
static inline __device__ _MaybeUnused _s__v3j __charm_sycl_vec_splat_v3j(unsigned int x){ return __charm_sycl_vec_v3j(x,x,x); }
static inline __device__ _MaybeUnused _s__v4j __charm_sycl_vec_splat_v4j(unsigned int x){ return __charm_sycl_vec_v4j(x,x,x,x); }
static inline __device__ _MaybeUnused _s__v8j __charm_sycl_vec_splat_v8j(unsigned int x){ return __charm_sycl_vec_v8j(x,x,x,x,x,x,x,x); }
static inline __device__ _MaybeUnused _s__v16j __charm_sycl_vec_splat_v16j(unsigned int x){ return __charm_sycl_vec_v16j(x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x); }
static inline __device__ _MaybeUnused _s__v1l __charm_sycl_vec_splat_v1l(long x){ return __charm_sycl_vec_v1l(x); }
static inline __device__ _MaybeUnused _s__v2l __charm_sycl_vec_splat_v2l(long x){ return __charm_sycl_vec_v2l(x,x); }
static inline __device__ _MaybeUnused _s__v3l __charm_sycl_vec_splat_v3l(long x){ return __charm_sycl_vec_v3l(x,x,x); }
static inline __device__ _MaybeUnused _s__v4l __charm_sycl_vec_splat_v4l(long x){ return __charm_sycl_vec_v4l(x,x,x,x); }
static inline __device__ _MaybeUnused _s__v8l __charm_sycl_vec_splat_v8l(long x){ return __charm_sycl_vec_v8l(x,x,x,x,x,x,x,x); }
static inline __device__ _MaybeUnused _s__v16l __charm_sycl_vec_splat_v16l(long x){ return __charm_sycl_vec_v16l(x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x); }
static inline __device__ _MaybeUnused _s__v1m __charm_sycl_vec_splat_v1m(unsigned long x){ return __charm_sycl_vec_v1m(x); }
static inline __device__ _MaybeUnused _s__v2m __charm_sycl_vec_splat_v2m(unsigned long x){ return __charm_sycl_vec_v2m(x,x); }
static inline __device__ _MaybeUnused _s__v3m __charm_sycl_vec_splat_v3m(unsigned long x){ return __charm_sycl_vec_v3m(x,x,x); }
static inline __device__ _MaybeUnused _s__v4m __charm_sycl_vec_splat_v4m(unsigned long x){ return __charm_sycl_vec_v4m(x,x,x,x); }
static inline __device__ _MaybeUnused _s__v8m __charm_sycl_vec_splat_v8m(unsigned long x){ return __charm_sycl_vec_v8m(x,x,x,x,x,x,x,x); }
static inline __device__ _MaybeUnused _s__v16m __charm_sycl_vec_splat_v16m(unsigned long x){ return __charm_sycl_vec_v16m(x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x); }
static inline __device__ _MaybeUnused _s__v1x __charm_sycl_vec_splat_v1x(long long x){ return __charm_sycl_vec_v1x(x); }
static inline __device__ _MaybeUnused _s__v2x __charm_sycl_vec_splat_v2x(long long x){ return __charm_sycl_vec_v2x(x,x); }
static inline __device__ _MaybeUnused _s__v3x __charm_sycl_vec_splat_v3x(long long x){ return __charm_sycl_vec_v3x(x,x,x); }
static inline __device__ _MaybeUnused _s__v4x __charm_sycl_vec_splat_v4x(long long x){ return __charm_sycl_vec_v4x(x,x,x,x); }
static inline __device__ _MaybeUnused _s__v8x __charm_sycl_vec_splat_v8x(long long x){ return __charm_sycl_vec_v8x(x,x,x,x,x,x,x,x); }
static inline __device__ _MaybeUnused _s__v16x __charm_sycl_vec_splat_v16x(long long x){ return __charm_sycl_vec_v16x(x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x); }
static inline __device__ _MaybeUnused _s__v1y __charm_sycl_vec_splat_v1y(unsigned long long x){ return __charm_sycl_vec_v1y(x); }
static inline __device__ _MaybeUnused _s__v2y __charm_sycl_vec_splat_v2y(unsigned long long x){ return __charm_sycl_vec_v2y(x,x); }
static inline __device__ _MaybeUnused _s__v3y __charm_sycl_vec_splat_v3y(unsigned long long x){ return __charm_sycl_vec_v3y(x,x,x); }
static inline __device__ _MaybeUnused _s__v4y __charm_sycl_vec_splat_v4y(unsigned long long x){ return __charm_sycl_vec_v4y(x,x,x,x); }
static inline __device__ _MaybeUnused _s__v8y __charm_sycl_vec_splat_v8y(unsigned long long x){ return __charm_sycl_vec_v8y(x,x,x,x,x,x,x,x); }
static inline __device__ _MaybeUnused _s__v16y __charm_sycl_vec_splat_v16y(unsigned long long x){ return __charm_sycl_vec_v16y(x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x); }
static inline __device__ _MaybeUnused _s__v1f __charm_sycl_vec_splat_v1f(float x){ return __charm_sycl_vec_v1f(x); }
static inline __device__ _MaybeUnused _s__v2f __charm_sycl_vec_splat_v2f(float x){ return __charm_sycl_vec_v2f(x,x); }
static inline __device__ _MaybeUnused _s__v3f __charm_sycl_vec_splat_v3f(float x){ return __charm_sycl_vec_v3f(x,x,x); }
static inline __device__ _MaybeUnused _s__v4f __charm_sycl_vec_splat_v4f(float x){ return __charm_sycl_vec_v4f(x,x,x,x); }
static inline __device__ _MaybeUnused _s__v8f __charm_sycl_vec_splat_v8f(float x){ return __charm_sycl_vec_v8f(x,x,x,x,x,x,x,x); }
static inline __device__ _MaybeUnused _s__v16f __charm_sycl_vec_splat_v16f(float x){ return __charm_sycl_vec_v16f(x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x); }
static inline __device__ _MaybeUnused _s__v1d __charm_sycl_vec_splat_v1d(double x){ return __charm_sycl_vec_v1d(x); }
static inline __device__ _MaybeUnused _s__v2d __charm_sycl_vec_splat_v2d(double x){ return __charm_sycl_vec_v2d(x,x); }
static inline __device__ _MaybeUnused _s__v3d __charm_sycl_vec_splat_v3d(double x){ return __charm_sycl_vec_v3d(x,x,x); }
static inline __device__ _MaybeUnused _s__v4d __charm_sycl_vec_splat_v4d(double x){ return __charm_sycl_vec_v4d(x,x,x,x); }
static inline __device__ _MaybeUnused _s__v8d __charm_sycl_vec_splat_v8d(double x){ return __charm_sycl_vec_v8d(x,x,x,x,x,x,x,x); }
static inline __device__ _MaybeUnused _s__v16d __charm_sycl_vec_splat_v16d(double x){ return __charm_sycl_vec_v16d(x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x); }

static inline __device__ _MaybeUnused char __charm_sycl_vec_ix_v1c(_s__v1c vec, int i) { return vec.__v[i]; }
static inline __device__ _MaybeUnused char __charm_sycl_vec_ix_v2c(_s__v2c vec, int i) { return vec.__v[i]; }
static inline __device__ _MaybeUnused char __charm_sycl_vec_ix_v3c(_s__v3c vec, int i) { return vec.__v[i]; }
static inline __device__ _MaybeUnused char __charm_sycl_vec_ix_v4c(_s__v4c vec, int i) { return vec.__v[i]; }
static inline __device__ _MaybeUnused char __charm_sycl_vec_ix_v8c(_s__v8c vec, int i) { return vec.__v[i]; }
static inline __device__ _MaybeUnused char __charm_sycl_vec_ix_v16c(_s__v16c vec, int i) { return vec.__v[i]; }
static inline __device__ _MaybeUnused unsigned char __charm_sycl_vec_ix_v1h(_s__v1h vec, int i) { return vec.__v[i]; }
static inline __device__ _MaybeUnused unsigned char __charm_sycl_vec_ix_v2h(_s__v2h vec, int i) { return vec.__v[i]; }
static inline __device__ _MaybeUnused unsigned char __charm_sycl_vec_ix_v3h(_s__v3h vec, int i) { return vec.__v[i]; }
static inline __device__ _MaybeUnused unsigned char __charm_sycl_vec_ix_v4h(_s__v4h vec, int i) { return vec.__v[i]; }
static inline __device__ _MaybeUnused unsigned char __charm_sycl_vec_ix_v8h(_s__v8h vec, int i) { return vec.__v[i]; }
static inline __device__ _MaybeUnused unsigned char __charm_sycl_vec_ix_v16h(_s__v16h vec, int i) { return vec.__v[i]; }
static inline __device__ _MaybeUnused short __charm_sycl_vec_ix_v1s(_s__v1s vec, int i) { return vec.__v[i]; }
static inline __device__ _MaybeUnused short __charm_sycl_vec_ix_v2s(_s__v2s vec, int i) { return vec.__v[i]; }
static inline __device__ _MaybeUnused short __charm_sycl_vec_ix_v3s(_s__v3s vec, int i) { return vec.__v[i]; }
static inline __device__ _MaybeUnused short __charm_sycl_vec_ix_v4s(_s__v4s vec, int i) { return vec.__v[i]; }
static inline __device__ _MaybeUnused short __charm_sycl_vec_ix_v8s(_s__v8s vec, int i) { return vec.__v[i]; }
static inline __device__ _MaybeUnused short __charm_sycl_vec_ix_v16s(_s__v16s vec, int i) { return vec.__v[i]; }
static inline __device__ _MaybeUnused unsigned short __charm_sycl_vec_ix_v1t(_s__v1t vec, int i) { return vec.__v[i]; }
static inline __device__ _MaybeUnused unsigned short __charm_sycl_vec_ix_v2t(_s__v2t vec, int i) { return vec.__v[i]; }
static inline __device__ _MaybeUnused unsigned short __charm_sycl_vec_ix_v3t(_s__v3t vec, int i) { return vec.__v[i]; }
static inline __device__ _MaybeUnused unsigned short __charm_sycl_vec_ix_v4t(_s__v4t vec, int i) { return vec.__v[i]; }
static inline __device__ _MaybeUnused unsigned short __charm_sycl_vec_ix_v8t(_s__v8t vec, int i) { return vec.__v[i]; }
static inline __device__ _MaybeUnused unsigned short __charm_sycl_vec_ix_v16t(_s__v16t vec, int i) { return vec.__v[i]; }
static inline __device__ _MaybeUnused int __charm_sycl_vec_ix_v1i(_s__v1i vec, int i) { return vec.__v[i]; }
static inline __device__ _MaybeUnused int __charm_sycl_vec_ix_v2i(_s__v2i vec, int i) { return vec.__v[i]; }
static inline __device__ _MaybeUnused int __charm_sycl_vec_ix_v3i(_s__v3i vec, int i) { return vec.__v[i]; }
static inline __device__ _MaybeUnused int __charm_sycl_vec_ix_v4i(_s__v4i vec, int i) { return vec.__v[i]; }
static inline __device__ _MaybeUnused int __charm_sycl_vec_ix_v8i(_s__v8i vec, int i) { return vec.__v[i]; }
static inline __device__ _MaybeUnused int __charm_sycl_vec_ix_v16i(_s__v16i vec, int i) { return vec.__v[i]; }
static inline __device__ _MaybeUnused unsigned int __charm_sycl_vec_ix_v1j(_s__v1j vec, int i) { return vec.__v[i]; }
static inline __device__ _MaybeUnused unsigned int __charm_sycl_vec_ix_v2j(_s__v2j vec, int i) { return vec.__v[i]; }
static inline __device__ _MaybeUnused unsigned int __charm_sycl_vec_ix_v3j(_s__v3j vec, int i) { return vec.__v[i]; }
static inline __device__ _MaybeUnused unsigned int __charm_sycl_vec_ix_v4j(_s__v4j vec, int i) { return vec.__v[i]; }
static inline __device__ _MaybeUnused unsigned int __charm_sycl_vec_ix_v8j(_s__v8j vec, int i) { return vec.__v[i]; }
static inline __device__ _MaybeUnused unsigned int __charm_sycl_vec_ix_v16j(_s__v16j vec, int i) { return vec.__v[i]; }
static inline __device__ _MaybeUnused long __charm_sycl_vec_ix_v1l(_s__v1l vec, int i) { return vec.__v[i]; }
static inline __device__ _MaybeUnused long __charm_sycl_vec_ix_v2l(_s__v2l vec, int i) { return vec.__v[i]; }
static inline __device__ _MaybeUnused long __charm_sycl_vec_ix_v3l(_s__v3l vec, int i) { return vec.__v[i]; }
static inline __device__ _MaybeUnused long __charm_sycl_vec_ix_v4l(_s__v4l vec, int i) { return vec.__v[i]; }
static inline __device__ _MaybeUnused long __charm_sycl_vec_ix_v8l(_s__v8l vec, int i) { return vec.__v[i]; }
static inline __device__ _MaybeUnused long __charm_sycl_vec_ix_v16l(_s__v16l vec, int i) { return vec.__v[i]; }
static inline __device__ _MaybeUnused unsigned long __charm_sycl_vec_ix_v1m(_s__v1m vec, int i) { return vec.__v[i]; }
static inline __device__ _MaybeUnused unsigned long __charm_sycl_vec_ix_v2m(_s__v2m vec, int i) { return vec.__v[i]; }
static inline __device__ _MaybeUnused unsigned long __charm_sycl_vec_ix_v3m(_s__v3m vec, int i) { return vec.__v[i]; }
static inline __device__ _MaybeUnused unsigned long __charm_sycl_vec_ix_v4m(_s__v4m vec, int i) { return vec.__v[i]; }
static inline __device__ _MaybeUnused unsigned long __charm_sycl_vec_ix_v8m(_s__v8m vec, int i) { return vec.__v[i]; }
static inline __device__ _MaybeUnused unsigned long __charm_sycl_vec_ix_v16m(_s__v16m vec, int i) { return vec.__v[i]; }
static inline __device__ _MaybeUnused long long __charm_sycl_vec_ix_v1x(_s__v1x vec, int i) { return vec.__v[i]; }
static inline __device__ _MaybeUnused long long __charm_sycl_vec_ix_v2x(_s__v2x vec, int i) { return vec.__v[i]; }
static inline __device__ _MaybeUnused long long __charm_sycl_vec_ix_v3x(_s__v3x vec, int i) { return vec.__v[i]; }
static inline __device__ _MaybeUnused long long __charm_sycl_vec_ix_v4x(_s__v4x vec, int i) { return vec.__v[i]; }
static inline __device__ _MaybeUnused long long __charm_sycl_vec_ix_v8x(_s__v8x vec, int i) { return vec.__v[i]; }
static inline __device__ _MaybeUnused long long __charm_sycl_vec_ix_v16x(_s__v16x vec, int i) { return vec.__v[i]; }
static inline __device__ _MaybeUnused unsigned long long __charm_sycl_vec_ix_v1y(_s__v1y vec, int i) { return vec.__v[i]; }
static inline __device__ _MaybeUnused unsigned long long __charm_sycl_vec_ix_v2y(_s__v2y vec, int i) { return vec.__v[i]; }
static inline __device__ _MaybeUnused unsigned long long __charm_sycl_vec_ix_v3y(_s__v3y vec, int i) { return vec.__v[i]; }
static inline __device__ _MaybeUnused unsigned long long __charm_sycl_vec_ix_v4y(_s__v4y vec, int i) { return vec.__v[i]; }
static inline __device__ _MaybeUnused unsigned long long __charm_sycl_vec_ix_v8y(_s__v8y vec, int i) { return vec.__v[i]; }
static inline __device__ _MaybeUnused unsigned long long __charm_sycl_vec_ix_v16y(_s__v16y vec, int i) { return vec.__v[i]; }
static inline __device__ _MaybeUnused float __charm_sycl_vec_ix_v1f(_s__v1f vec, int i) { return vec.__v[i]; }
static inline __device__ _MaybeUnused float __charm_sycl_vec_ix_v2f(_s__v2f vec, int i) { return vec.__v[i]; }
static inline __device__ _MaybeUnused float __charm_sycl_vec_ix_v3f(_s__v3f vec, int i) { return vec.__v[i]; }
static inline __device__ _MaybeUnused float __charm_sycl_vec_ix_v4f(_s__v4f vec, int i) { return vec.__v[i]; }
static inline __device__ _MaybeUnused float __charm_sycl_vec_ix_v8f(_s__v8f vec, int i) { return vec.__v[i]; }
static inline __device__ _MaybeUnused float __charm_sycl_vec_ix_v16f(_s__v16f vec, int i) { return vec.__v[i]; }
static inline __device__ _MaybeUnused double __charm_sycl_vec_ix_v1d(_s__v1d vec, int i) { return vec.__v[i]; }
static inline __device__ _MaybeUnused double __charm_sycl_vec_ix_v2d(_s__v2d vec, int i) { return vec.__v[i]; }
static inline __device__ _MaybeUnused double __charm_sycl_vec_ix_v3d(_s__v3d vec, int i) { return vec.__v[i]; }
static inline __device__ _MaybeUnused double __charm_sycl_vec_ix_v4d(_s__v4d vec, int i) { return vec.__v[i]; }
static inline __device__ _MaybeUnused double __charm_sycl_vec_ix_v8d(_s__v8d vec, int i) { return vec.__v[i]; }
static inline __device__ _MaybeUnused double __charm_sycl_vec_ix_v16d(_s__v16d vec, int i) { return vec.__v[i]; }

static inline __device__ _MaybeUnused void __charm_sycl_vec_aS_v1c(_s__v1c* vec, int i, char v){vec->__v[i]=v;}
static inline __device__ _MaybeUnused void __charm_sycl_vec_aS_v2c(_s__v2c* vec, int i, char v){vec->__v[i]=v;}
static inline __device__ _MaybeUnused void __charm_sycl_vec_aS_v3c(_s__v3c* vec, int i, char v){vec->__v[i]=v;}
static inline __device__ _MaybeUnused void __charm_sycl_vec_aS_v4c(_s__v4c* vec, int i, char v){vec->__v[i]=v;}
static inline __device__ _MaybeUnused void __charm_sycl_vec_aS_v8c(_s__v8c* vec, int i, char v){vec->__v[i]=v;}
static inline __device__ _MaybeUnused void __charm_sycl_vec_aS_v16c(_s__v16t* vec, int i, char v){vec->__v[i]=v;}
static inline __device__ _MaybeUnused void __charm_sycl_vec_aS_v1h(_s__v1h* vec, int i, unsigned char v){vec->__v[i]=v;}
static inline __device__ _MaybeUnused void __charm_sycl_vec_aS_v2h(_s__v2h* vec, int i, unsigned char v){vec->__v[i]=v;}
static inline __device__ _MaybeUnused void __charm_sycl_vec_aS_v3h(_s__v3h* vec, int i, unsigned char v){vec->__v[i]=v;}
static inline __device__ _MaybeUnused void __charm_sycl_vec_aS_v4h(_s__v4h* vec, int i, unsigned char v){vec->__v[i]=v;}
static inline __device__ _MaybeUnused void __charm_sycl_vec_aS_v8h(_s__v8h* vec, int i, unsigned char v){vec->__v[i]=v;}
static inline __device__ _MaybeUnused void __charm_sycl_vec_aS_v16h(_s__v16t* vec, int i, unsigned char v){vec->__v[i]=v;}
static inline __device__ _MaybeUnused void __charm_sycl_vec_aS_v1s(_s__v1s* vec, int i, short v){vec->__v[i]=v;}
static inline __device__ _MaybeUnused void __charm_sycl_vec_aS_v2s(_s__v2s* vec, int i, short v){vec->__v[i]=v;}
static inline __device__ _MaybeUnused void __charm_sycl_vec_aS_v3s(_s__v3s* vec, int i, short v){vec->__v[i]=v;}
static inline __device__ _MaybeUnused void __charm_sycl_vec_aS_v4s(_s__v4s* vec, int i, short v){vec->__v[i]=v;}
static inline __device__ _MaybeUnused void __charm_sycl_vec_aS_v8s(_s__v8s* vec, int i, short v){vec->__v[i]=v;}
static inline __device__ _MaybeUnused void __charm_sycl_vec_aS_v16s(_s__v16t* vec, int i, short v){vec->__v[i]=v;}
static inline __device__ _MaybeUnused void __charm_sycl_vec_aS_v1t(_s__v1t* vec, int i, unsigned short v){vec->__v[i]=v;}
static inline __device__ _MaybeUnused void __charm_sycl_vec_aS_v2t(_s__v2t* vec, int i, unsigned short v){vec->__v[i]=v;}
static inline __device__ _MaybeUnused void __charm_sycl_vec_aS_v3t(_s__v3t* vec, int i, unsigned short v){vec->__v[i]=v;}
static inline __device__ _MaybeUnused void __charm_sycl_vec_aS_v4t(_s__v4t* vec, int i, unsigned short v){vec->__v[i]=v;}
static inline __device__ _MaybeUnused void __charm_sycl_vec_aS_v8t(_s__v8t* vec, int i, unsigned short v){vec->__v[i]=v;}
static inline __device__ _MaybeUnused void __charm_sycl_vec_aS_v16t(_s__v16t* vec, int i, unsigned short v){vec->__v[i]=v;}
static inline __device__ _MaybeUnused void __charm_sycl_vec_aS_v1i(_s__v1i* vec, int i, int v){vec->__v[i]=v;}
static inline __device__ _MaybeUnused void __charm_sycl_vec_aS_v2i(_s__v2i* vec, int i, int v){vec->__v[i]=v;}
static inline __device__ _MaybeUnused void __charm_sycl_vec_aS_v3i(_s__v3i* vec, int i, int v){vec->__v[i]=v;}
static inline __device__ _MaybeUnused void __charm_sycl_vec_aS_v4i(_s__v4i* vec, int i, int v){vec->__v[i]=v;}
static inline __device__ _MaybeUnused void __charm_sycl_vec_aS_v8i(_s__v8i* vec, int i, int v){vec->__v[i]=v;}
static inline __device__ _MaybeUnused void __charm_sycl_vec_aS_v16i(_s__v16t* vec, int i, int v){vec->__v[i]=v;}
static inline __device__ _MaybeUnused void __charm_sycl_vec_aS_v1j(_s__v1j* vec, int i, unsigned int v){vec->__v[i]=v;}
static inline __device__ _MaybeUnused void __charm_sycl_vec_aS_v2j(_s__v2j* vec, int i, unsigned int v){vec->__v[i]=v;}
static inline __device__ _MaybeUnused void __charm_sycl_vec_aS_v3j(_s__v3j* vec, int i, unsigned int v){vec->__v[i]=v;}
static inline __device__ _MaybeUnused void __charm_sycl_vec_aS_v4j(_s__v4j* vec, int i, unsigned int v){vec->__v[i]=v;}
static inline __device__ _MaybeUnused void __charm_sycl_vec_aS_v8j(_s__v8j* vec, int i, unsigned int v){vec->__v[i]=v;}
static inline __device__ _MaybeUnused void __charm_sycl_vec_aS_v16j(_s__v16t* vec, int i, unsigned int v){vec->__v[i]=v;}
static inline __device__ _MaybeUnused void __charm_sycl_vec_aS_v1l(_s__v1l* vec, int i, long v){vec->__v[i]=v;}
static inline __device__ _MaybeUnused void __charm_sycl_vec_aS_v2l(_s__v2l* vec, int i, long v){vec->__v[i]=v;}
static inline __device__ _MaybeUnused void __charm_sycl_vec_aS_v3l(_s__v3l* vec, int i, long v){vec->__v[i]=v;}
static inline __device__ _MaybeUnused void __charm_sycl_vec_aS_v4l(_s__v4l* vec, int i, long v){vec->__v[i]=v;}
static inline __device__ _MaybeUnused void __charm_sycl_vec_aS_v8l(_s__v8l* vec, int i, long v){vec->__v[i]=v;}
static inline __device__ _MaybeUnused void __charm_sycl_vec_aS_v16l(_s__v16t* vec, int i, long v){vec->__v[i]=v;}
static inline __device__ _MaybeUnused void __charm_sycl_vec_aS_v1m(_s__v1m* vec, int i, unsigned long v){vec->__v[i]=v;}
static inline __device__ _MaybeUnused void __charm_sycl_vec_aS_v2m(_s__v2m* vec, int i, unsigned long v){vec->__v[i]=v;}
static inline __device__ _MaybeUnused void __charm_sycl_vec_aS_v3m(_s__v3m* vec, int i, unsigned long v){vec->__v[i]=v;}
static inline __device__ _MaybeUnused void __charm_sycl_vec_aS_v4m(_s__v4m* vec, int i, unsigned long v){vec->__v[i]=v;}
static inline __device__ _MaybeUnused void __charm_sycl_vec_aS_v8m(_s__v8m* vec, int i, unsigned long v){vec->__v[i]=v;}
static inline __device__ _MaybeUnused void __charm_sycl_vec_aS_v16m(_s__v16t* vec, int i, unsigned long v){vec->__v[i]=v;}
static inline __device__ _MaybeUnused void __charm_sycl_vec_aS_v1x(_s__v1x* vec, int i, long long v){vec->__v[i]=v;}
static inline __device__ _MaybeUnused void __charm_sycl_vec_aS_v2x(_s__v2x* vec, int i, long long v){vec->__v[i]=v;}
static inline __device__ _MaybeUnused void __charm_sycl_vec_aS_v3x(_s__v3x* vec, int i, long long v){vec->__v[i]=v;}
static inline __device__ _MaybeUnused void __charm_sycl_vec_aS_v4x(_s__v4x* vec, int i, long long v){vec->__v[i]=v;}
static inline __device__ _MaybeUnused void __charm_sycl_vec_aS_v8x(_s__v8x* vec, int i, long long v){vec->__v[i]=v;}
static inline __device__ _MaybeUnused void __charm_sycl_vec_aS_v16x(_s__v16t* vec, int i, long long v){vec->__v[i]=v;}
static inline __device__ _MaybeUnused void __charm_sycl_vec_aS_v1y(_s__v1y* vec, int i, unsigned long long v){vec->__v[i]=v;}
static inline __device__ _MaybeUnused void __charm_sycl_vec_aS_v2y(_s__v2y* vec, int i, unsigned long long v){vec->__v[i]=v;}
static inline __device__ _MaybeUnused void __charm_sycl_vec_aS_v3y(_s__v3y* vec, int i, unsigned long long v){vec->__v[i]=v;}
static inline __device__ _MaybeUnused void __charm_sycl_vec_aS_v4y(_s__v4y* vec, int i, unsigned long long v){vec->__v[i]=v;}
static inline __device__ _MaybeUnused void __charm_sycl_vec_aS_v8y(_s__v8y* vec, int i, unsigned long long v){vec->__v[i]=v;}
static inline __device__ _MaybeUnused void __charm_sycl_vec_aS_v16y(_s__v16t* vec, int i, unsigned long long v){vec->__v[i]=v;}
static inline __device__ _MaybeUnused void __charm_sycl_vec_aS_v1f(_s__v1f* vec, int i, float v){vec->__v[i]=v;}
static inline __device__ _MaybeUnused void __charm_sycl_vec_aS_v2f(_s__v2f* vec, int i, float v){vec->__v[i]=v;}
static inline __device__ _MaybeUnused void __charm_sycl_vec_aS_v3f(_s__v3f* vec, int i, float v){vec->__v[i]=v;}
static inline __device__ _MaybeUnused void __charm_sycl_vec_aS_v4f(_s__v4f* vec, int i, float v){vec->__v[i]=v;}
static inline __device__ _MaybeUnused void __charm_sycl_vec_aS_v8f(_s__v8f* vec, int i, float v){vec->__v[i]=v;}
static inline __device__ _MaybeUnused void __charm_sycl_vec_aS_v16f(_s__v16t* vec, int i, float v){vec->__v[i]=v;}
static inline __device__ _MaybeUnused void __charm_sycl_vec_aS_v1d(_s__v1d* vec, int i, double v){vec->__v[i]=v;}
static inline __device__ _MaybeUnused void __charm_sycl_vec_aS_v2d(_s__v2d* vec, int i, double v){vec->__v[i]=v;}
static inline __device__ _MaybeUnused void __charm_sycl_vec_aS_v3d(_s__v3d* vec, int i, double v){vec->__v[i]=v;}
static inline __device__ _MaybeUnused void __charm_sycl_vec_aS_v4d(_s__v4d* vec, int i, double v){vec->__v[i]=v;}
static inline __device__ _MaybeUnused void __charm_sycl_vec_aS_v8d(_s__v8d* vec, int i, double v){vec->__v[i]=v;}
static inline __device__ _MaybeUnused void __charm_sycl_vec_aS_v16d(_s__v16t* vec, int i, double v){vec->__v[i]=v;}

#undef _MaybeUnused

#ifdef __device__defined
#undef __device__
#undef __device__defined
#endif
)";
}
