#pragma once

//
// Created by prashant on 11/08/17.
//

#ifndef SHADER_HELPER_H
#define SHADER_HELPER_H

#include <fstream>
#include <sstream>


inline bool try_compile_shader(GLenum shaderHandle, const std::string& code)
{
	int codeLength = code.length();
	const char *bufPtr = code.c_str();
	glShaderSource(shaderHandle, 1, &bufPtr, &codeLength);
	glCompileShader(shaderHandle);

	int compileStatus;
	glGetShaderiv(shaderHandle, GL_COMPILE_STATUS, &compileStatus);

	if (!compileStatus)
	{
		int infoLogLength;
		glGetShaderiv(shaderHandle, GL_INFO_LOG_LENGTH, &infoLogLength);

		char *infoLog = new char[infoLogLength];
		glGetShaderInfoLog(shaderHandle, infoLogLength, nullptr, infoLog);

		int shaderType;
		const char *shaderTypeStr = "unknown";
		glGetShaderiv(shaderHandle, GL_SHADER_TYPE, &shaderType);

		switch (shaderType)
		{
		case GL_VERTEX_SHADER:
			shaderTypeStr = "vertex";
			break;
		case GL_TESS_CONTROL_SHADER:
			shaderTypeStr = "tessellation control";
			break;
		case GL_TESS_EVALUATION_SHADER:
			shaderTypeStr = "tessellation evaluation";
			break;
		case GL_GEOMETRY_SHADER:
			shaderTypeStr = "geometry";
			break;
		case GL_FRAGMENT_SHADER:
			shaderTypeStr = "fragment";
			break;
		}

		std::cerr << "Failed to compile " << shaderTypeStr << " shader!" << std::endl;
		std::cerr << infoLog << std::endl;

		delete infoLog;
		return false;
	}

	return true;
}

inline bool try_compile_shader_from_file(GLenum shaderHandle, const std::string& path)
{
	std::ifstream fin(path);

	if (!fin.is_open())
	{
		std::cerr << "Unable to open: " << path << std::endl;
		return false;
	}

	std::stringstream buffer;
	buffer << fin.rdbuf();

	fin.close();

	return try_compile_shader(shaderHandle, buffer.str());
}

inline bool try_link_program(GLenum programHandle)
{
	glLinkProgram(programHandle);
	int linkStatus;
	glGetProgramiv(programHandle, GL_LINK_STATUS, &linkStatus);

	if (!linkStatus)
	{
		int infoLogLength;
		glGetProgramiv(programHandle, GL_INFO_LOG_LENGTH, &infoLogLength);

		char *infoLog = new char[infoLogLength];
		glGetProgramInfoLog(programHandle, infoLogLength, nullptr, infoLog);

		std::cerr << "Failed to link OpenGL program!" << std::endl;
		std::cerr << infoLog << std::endl;

		delete infoLog;
		return false;
	}

	return true;
}

#endif //SHADER_HELPER_H
