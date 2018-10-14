#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <fstream>
#include <CL/opencl.h>
#include "Lab4.h"

const char * CL_FILE_NAME = "Lab4.cl";

#pragma region Init

void initBoard(int n, int m, float* in) {
	for (auto i = 0; i < n; ++i) {
		for (auto j = 0; j < m; ++j) {
			if (i == 0
				|| j == 0
				|| i == n - 1
				|| j == m - 1)
				in[j + i*m] = 0.0f;
			else
				in[j + i*m] = static_cast<float>(i)*(n - static_cast<float>(i) - 1)*(static_cast<float>(j)*(m - static_cast<float>(j) - 1));
		}
	}
}

#pragma endregion

#pragma region Error

void onError(cl_int status_code, const char * message) {
	if (status_code != CL_SUCCESS) {
		std::cout << "Something went wrong: " << message << std::endl;
#ifdef PAUSE
		system("pause");
#endif
	}
}

void onError(cl_int status_code, const std::string message) {
	if (status_code != CL_SUCCESS) {
		std::cout << "Something went wrong: " << message << std::endl;
#ifdef PAUSE
		system("pause");
#endif
	}
}

bool validate_clBuildProgram(cl_int err, const cl_program program, cl_device_id* device_id) {
	if (err == CL_SUCCESS)
		return true;

	size_t len;
	char buffer[204800];
	cl_build_status bldstatus;
	printf("\nError %d: Failed to build program executable [ ]\n", err);
	err = clGetProgramBuildInfo(program, *device_id, CL_PROGRAM_BUILD_STATUS, sizeof(bldstatus), (void *)&bldstatus, &len);
	if (err != CL_SUCCESS) {
		printf("Build Status error %d: %s\n", err, "");
		exit(1);
	}
	if (bldstatus == CL_BUILD_SUCCESS) printf("Build Status: CL_BUILD_SUCCESS\n");
	if (bldstatus == CL_BUILD_NONE) printf("Build Status: CL_BUILD_NONE\n");
	if (bldstatus == CL_BUILD_ERROR) printf("Build Status: CL_BUILD_ERROR\n");
	if (bldstatus == CL_BUILD_IN_PROGRESS) printf("Build Status: CL_BUILD_IN_PROGRESS\n");
#ifdef PAUSE
	system("pause");
#endif
	err = clGetProgramBuildInfo(program, *device_id, CL_PROGRAM_BUILD_OPTIONS, sizeof(buffer), buffer, &len);
	if (err != CL_SUCCESS) {
		printf("Build Options error %d: %s\n", err, "");
		exit(1);
	}
	printf("Build Options: %s\n", buffer);
#ifdef PAUSE
	system("pause");
#endif
	err = clGetProgramBuildInfo(program, *device_id, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &len);
	if (err != CL_SUCCESS) {
		printf("Build Log error %d: %s\n", err, "");
		exit(1);
	}
	printf("Build Log:\n%s\n", buffer);
#ifdef PAUSE
	system("pause");
#endif
	return false;
}

#pragma endregion

#pragma region OpenCl

const char* load_program_source(const char *filename) {
	std::string line;
	std::ostringstream alllines;
	std::ifstream fileStream(filename);
	if (fileStream.is_open()) {
		while (std::getline(fileStream, line)) {
			alllines << line;
		}
		fileStream.close();
	}
	else {
		std::cout << "Couldn't read the Kernel File";
	}

	auto sourceString = alllines.str();
	auto source = new char[sourceString.length() + 1];
	strcpy(source, sourceString.c_str());
	return source;
}

bool getMemSizeOfDeviceAndCreateContext(cl_device_id* device_id, unsigned __int64* memory_size, cl_context* context) {
	auto ok = true;

	cl_platform_id selected_platform = nullptr;
	cl_device_id *devices = 0;
	cl_uint device_count = 0;
	cl_ulong memsize = 0;
	cl_int errnum = 0;

	// Platform Count
	cl_uint num_platforms;
	clGetPlatformIDs(0, nullptr, &num_platforms);
	if (num_platforms == 0) {
		std::cout << "No platforms could be found!" << std::endl;
		ok = false;
	}

	// Platform Info
	if (ok) {
		cl_platform_id* platforms = 0;

		platforms = static_cast<cl_platform_id*>(malloc(num_platforms * sizeof(cl_platform_id)));
		clGetPlatformIDs(num_platforms, platforms, nullptr);

		char chBuffer[1024];
		for (cl_uint i = 0; i < num_platforms; ++i) {
			errnum = clGetPlatformInfo(platforms[i], CL_PLATFORM_NAME, 1024, &chBuffer, nullptr);
			if (errnum == CL_SUCCESS) {
				if (strstr(chBuffer, "NVIDIA") != nullptr) {
					selected_platform = platforms[i];
				}
			}
		}
		if (selected_platform == nullptr) {
			std::cout << "No NVIDIA could be found. Default-ing to the first platform" << std::endl;
			selected_platform = platforms[0];
		}

		free(platforms);
	}

	// Platform Devices
	if (ok) {
		errnum = clGetDeviceIDs(selected_platform, CL_DEVICE_TYPE_GPU, 0, nullptr, &device_count);
		devices = static_cast<cl_device_id *>(malloc(device_count * sizeof(cl_device_id)));
		errnum = clGetDeviceIDs(selected_platform, CL_DEVICE_TYPE_GPU, device_count, devices, nullptr);
		if (errnum != CL_SUCCESS) {
			std::cout << "Platform Devices couldn't be retrieved" << std::endl;
			ok = false;
		}
	}

	// Select device
	auto selected_device = 0;
	*device_id = devices[selected_device];

	// Create OpenCL Context
	if (ok) {
		*context = clCreateContext(0, device_count, device_id, nullptr, nullptr, &errnum);
		if (*context == static_cast<cl_context>(0)) {
			std::cout << "Context couldn't be defined" << std::endl;
			std::cout << device_count << std::endl;
			std::cout << device_id << std::endl;
			if(errnum == CL_INVALID_PLATFORM)
			{
				std::cout << "Invalid Plarform(" << errnum << ")" << std::endl;
			}
			if (errnum == CL_INVALID_VALUE)
			{
				std::cout << "Invalid Value(" << errnum << ")" << std::endl;
			}
			if (errnum == CL_INVALID_DEVICE)
			{
				std::cout << "Invalid Device(" << errnum << ")" << std::endl;
			}
			if (errnum == CL_DEVICE_NOT_AVAILABLE)
			{
				std::cout << "Device not available(" << errnum << ")" << std::endl;
			}
			if (errnum == CL_OUT_OF_HOST_MEMORY)
			{
				std::cout << "Out Of Host Memory(" << errnum << ")" << std::endl;
			}
			ok = false;
		}
	}

	// Get Max Memory Allocation
	if (ok) {
		errnum = clGetDeviceInfo(devices[selected_device], CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(cl_ulong), &memsize, nullptr);
		if (errnum != CL_SUCCESS) {
			std::cout << "Couldn't retrieve the device information to have the memory size" << std::endl;
			ok = false;
		}
		else {
			// We should have the Memory allocation
			*memory_size = static_cast<unsigned __int64>(memsize);
		}
	}

	return ok;
}

#pragma endregion

float open_cl(unsigned int m, unsigned int n, int np, float tdh, float c) {
	// Load Kernel File
	auto KernelSource = load_program_source(CL_FILE_NAME);

	// Allocate Arrays
	int size = n * m;
	auto mem_size_matrix = size * sizeof(float);
	auto in = new float[size];
	auto* out = new float[size];

	memset(in, 0, mem_size_matrix);
	memset(out, 0, mem_size_matrix);

	// Init Board
	initBoard(n, m, in);

#ifdef PRINT_INITIAL_MATRIX
	printf(INITIAL_MATRIX_MESSAGE, "Parallel");
	print1DMatrix(n, m, in);
#endif

	// Start Timer
	SYSTEMTIME time;
	GetSystemTime(&time);
	auto startTime = (time.wSecond * 1000) + time.wMilliseconds;

	bool ok = true;
	cl_int err;

	// Get DeviceID, Memory Size and Context
	cl_context context;
	cl_device_id device_id;
	unsigned __int64 memory_size;
	ok = getMemSizeOfDeviceAndCreateContext(&device_id, &memory_size, &context);
	if (!ok) {
		return 0;
	}

	// Create Work Queue
	cl_command_queue commands;
	commands = clCreateCommandQueue(context, device_id, 0, &err);
	onError(err, "Couldn't create the Command Queue");

	// Create OpenCL memory for both arrays
	cl_mem input = clCreateBuffer(context, CL_MEM_READ_ONLY, mem_size_matrix, nullptr, &err);
	onError(err, "Input Array Buffer couldn't be allocated in memory.");
	onError(err, "TDHMEM couldn't be allocated in memory.");
	cl_mem output = clCreateBuffer(context, CL_MEM_WRITE_ONLY, mem_size_matrix, nullptr, &err);
	onError(err, "Output Array Buffer couldn't be allocated in memory.");

	// Create and Build the Kernel
	cl_program program;

	const size_t kernel_source_length = strlen(KernelSource);
	program = clCreateProgramWithSource(context, 1, static_cast<const char **>(&KernelSource), &kernel_source_length, &err);
	onError(err, "Couldn't Create the Program with the provided Kernel");
	err = clBuildProgram(program, 1, &device_id, nullptr, nullptr, nullptr);
	onError(err, "Couldn't build the program.");

	// TODO: Remove
	validate_clBuildProgram(err, program, &device_id);
	onError(err, "Couldn't validate the build of the program.");

	// Enqueue write to GPU
	err = clEnqueueWriteBuffer(commands, input, CL_FALSE, 0, mem_size_matrix, in, 0, nullptr, nullptr);
	onError(err, "Input Array couldn't be Enqueued to be written in the GPU's Global Memory.");
	err = clEnqueueWriteBuffer(commands, output, CL_FALSE, 0, mem_size_matrix, out, 0, nullptr, nullptr);
	onError(err, "Output Array couldn't be Enqueued to be written in the GPU's Global Memory.");

	// Create the KERNEL Obj
	cl_kernel kernel;
	kernel = clCreateKernel(program, "calculerPlaque", &err);
#ifdef DEBUG // Kernel
	onError(err, "Couldn't create the Kernel.");
	if(err == CL_INVALID_PROGRAM)
	{
		std::cout << "Kernel Error: Invalid Program" << std::endl;
	}
	if (err == CL_INVALID_PROGRAM_EXECUTABLE)
	{
		std::cout << "Kernel Error: Invalid Program Executable" << std::endl;
	}
	if (err == CL_INVALID_KERNEL_NAME)
	{
		std::cout << "Kernel Error: Invalid Kernel Name" << std::endl;
		std::cout << &KernelSource << std::endl;
	}
	if (err == CL_INVALID_KERNEL_DEFINITION)
	{
		std::cout << "Kernel Error: Invalid Kernel Definition" << std::endl;
	}
	if (err == CL_INVALID_VALUE)
	{
		std::cout << "Kernel Error: Invalid Value" << std::endl;
	}
	if (err == CL_OUT_OF_HOST_MEMORY)
	{
		std::cout << "Kernel Error: Out Of Host Memory" << std::endl;
	}
#endif
	
	int argi = 0;
	clSetKernelArg(kernel, argi++, sizeof(cl_uint), static_cast<void*>(&m));
	clSetKernelArg(kernel, argi++, sizeof(cl_float), static_cast<void*>(&c));
	clSetKernelArg(kernel, argi++, sizeof(cl_float), static_cast<void*>(&tdh));
	clSetKernelArg(kernel, argi++, sizeof(cl_mem), static_cast<void*>(&input));
	clSetKernelArg(kernel, argi++, sizeof(cl_mem), static_cast<void*>(&output));
	clSetKernelArg(kernel, argi++, sizeof(cl_uint), static_cast<void*>(&size));

	// Create Work Group
	size_t global;
	size_t local;
	err = clGetKernelWorkGroupInfo(kernel, device_id, CL_KERNEL_WORK_GROUP_SIZE, sizeof(local), &local, nullptr);
	onError(err, "Couldn't retrieve the Work Group Size");
	global = (n % local == 0) ? n : n + local - (n % local);

	// Enqueue ND Range Kernel	
	for (int k = 0; k < np; k++)
		err |= clEnqueueNDRangeKernel(commands, kernel, 1, nullptr, &global, &local, 0, nullptr, nullptr);
	std::ostringstream fmt;
	fmt << "Couldn't Enqueue the Kernel (Error " << err << ")";
	onError(err, static_cast<std::string>(fmt.str()));

	// Finish
	clFinish(commands);

	// Read Output
	err = clEnqueueReadBuffer(commands, output, CL_TRUE, 0, mem_size_matrix, out, 0, nullptr, nullptr);
	onError(err, "Couldn't Read the output");

	// End Timer
	GetSystemTime(&time);
	auto endTime = (time.wSecond * 1000) + time.wMilliseconds;
	auto diff = endTime - startTime;
	//

#ifdef PRINT_FINAL_MATRIX
	printf(FINAL_MATRIX_MESSAGE, "Parallel", np);
	print1DMatrix(n, m, out);
#endif

	delete[] in;
	delete[] out;

	return diff;
}