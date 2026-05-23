#ifndef ERESULT_H
#define ERESULT_H
#pragma once
//-----------------------------------------------------------------------------------
//
//	EResult.h
//		Declaration and definition of ERESULT error return codes
//		Note: This is compatible with Windows HRESULT
//-----------------------------------------------------------------------------------
#ifdef WIN32
// VS2008 does not have <cstdint>
typedef __int32	ERESULT;
#else
#include <cstdint>
typedef int32_t	ERESULT;
#endif

inline bool		Success( ERESULT hr ) { return (hr >= 0); }
inline bool		Failure( ERESULT hr ) { return (hr < 0); }

#define NOERROR_				0
#define E_UNEXPECTED_		0x8000FFFF
#define E_NOTIMPL_			0x80004001
#define E_OUTOFMEMORY_		0x8007000E
#define E_INVALIDARG_		0x80070057
#define E_FAIL_				0x80004005
#define E_ACCESSDENIED_		0x80070005

// ERROR_BAD_FORMAT passes Success() test.  We usually use it to flag a loss of precision
#define E_ERROR_BAD_FORMAT_	11
#define E_ERROR_ARITHMETIC_OVERFLOW_ 534
#endif // ERESULT_H
