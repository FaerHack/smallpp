#pragma once
#include <string>

#include "shared/types.h"

class c_generator {
private:
	std::wstring m_path;

public:
	c_generator( const std::wstring& path );

	bool generate( proto_file_s& proto_file );
};