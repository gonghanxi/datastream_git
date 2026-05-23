#ifndef SYSTEMVUE_H
#define SYSTEMVUE_H
#pragma once

//导出定义
#if defined DISABLE_SYSTEMVUEMODELBUILDER_EXPORTS
# define SYSTEMVUEMODELBUILDER_API
#else
# ifdef _WIN32
#  if defined SYSTEMVUEMODELBUILDER_EXPORTS
#   define SYSTEMVUEMODELBUILDER_API __declspec(dllexport)
#  else
#   define SYSTEMVUEMODELBUILDER_API __declspec(dllimport)
#   define SYSTEMVUEMODELBUILDER_API_IMPORT
#  endif
# else
#   define SYSTEMVUEMODELBUILDER_API __attribute__ ((visibility ("default")))
# endif
#endif
#endif // SYSTEMVUE_H
