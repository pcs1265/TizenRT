/****************************************************************************
 *
 * Copyright 2016 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ****************************************************************************/
//***************************************************************************
// examples/helloxx/helloxx_main.cxx
//
//   Copyright (C) 2009, 2011-2013 Gregory Nutt. All rights reserved.
//   Author: Gregory Nutt <gnutt@nuttx.org>
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in
//    the documentation and/or other materials provided with the
//    distribution.
// 3. Neither the name NuttX nor the names of its contributors may be
//    used to endorse or promote products derived from this software
//    without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
// OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
// AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
// ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
//***************************************************************************

//***************************************************************************
// Included Files
//***************************************************************************

#include <tinyara/config.h>

#include <cstdio>
#include <debug.h>

#include <iostream>
// #include <sstream>

// #include <string>
// #include <algorithm>
// #include <vector>
// #include <random>

#include <tinyara/init.h>

//***************************************************************************
// Definitions
//***************************************************************************
// Debug ********************************************************************
// Non-standard debug that may be enabled just for testing the constructors

#ifndef CONFIG_DEBUG
#undef CONFIG_DEBUG_CXX
#endif

#ifdef CONFIG_DEBUG_CXX
#define cxxdbg dbg
#define cxxlldbg lldbg
#ifdef CONFIG_DEBUG_VERBOSE
#define cxxvdbg vdbg
#define cxxllvdbg llvdbg
#else
#define cxxvdbg(...)
#define cxxllvdbg(...)
#endif
#else
#define cxxdbg(...)
#define cxxlldbg(...)
#define cxxvdbg(...)
#define cxxllvdbg(...)
#endif

//***************************************************************************
// Private Classes
//***************************************************************************

//***************************************************************************
// Private Data
//***************************************************************************

//***************************************************************************
// Public Functions
//***************************************************************************

/****************************************************************************
 * Name: helloxx_main
 ****************************************************************************/

// // 1. 랜덤 데이터 생성 함수
// std::vector<int> generate_random_data(size_t size, int min_val = 1, int max_val = 1000)
// {
// 	std::random_device rd;
// 	std::mt19937 gen(rd());
// 	std::uniform_int_distribution<int> dist(min_val, max_val);

// 	std::vector<int> data(size);
// 	for (auto &num : data) {
// 		num = dist(gen);
// 	}
// 	return data;
// }

// // 2. 정렬 성능 측정 함수
// template <typename Func>
// void measure_sort(const std::string &name, std::vector<int> data, Func sort_func)
// {
// 	auto start = std::chrono::high_resolution_clock::now();
// 	std::sort(data.begin(), data.end(), sort_func);
// 	auto end = std::chrono::high_resolution_clock::now();

// 	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
// 	std::cout << name << " time: " << duration.count() << " μs" << std::endl;

// 	// 정렬 결과 검증 (오름차순 확인)
// 	bool is_sorted = std::is_sorted(data.begin(), data.end(), sort_func);
// 	std::cout << "Is sorted correctly? " << (is_sorted ? "Yes" : "No") << std::endl << std::endl;
// }

// // 3. 구조체 정의 및 정렬 예제
// struct Employee {
// 	int id;
// 	std::string name;
// 	double salary;

// 	Employee(int i, std::string n, double s) :
// 		id(i), name(std::move(n)), salary(s)
// 	{
// 	}
// };

// void sort_employees()
// {
// 	std::vector<Employee> employees = {
// 		{101, "Alice", 75000.0},
// 		{102, "Bob", 50000.0},
// 		{103, "Charlie", 90000.0},
// 		{104, "Diana", 60000.0}};

// 	// 급여 기준 내림차순 정렬
// 	std::sort(employees.begin(), employees.end(), [](const Employee &a, const Employee &b) {
// 		return a.salary > b.salary;
// 	});

// 	std::cout << "Employees sorted by salary (descending):\n";
// 	for (const auto &emp : employees) {
// 		std::cout << emp.id << ": " << emp.name << " ($" << emp.salary << ")" << std::endl;
// 	}
// }


// bool ccustom_sort(int a, int b) {
// 	if (a % 2 == 0 && b % 2 != 0)
// 		return true;
// 	if (a % 2 != 0 && b % 2 == 0)
// 		return false;
// 	if (a % 2 == 0)
// 		return a < b;
// 	return a > b;
// }

extern "C" {

int helloxx_main(int argc, char *argv[])
{
	// const size_t data_size = 10000;
	// auto data = generate_random_data(data_size);

	// // 1. 기본 오름차순 정렬
	// measure_sort("Default sort (ascending)", data, std::greater<int>());

	// // 2. 내림차순 정렬
	// measure_sort("Descending sort", data, std::less<int>());

	// // 3. 커스텀 정렬 (짝수 우선 → 오름차순, 홀수 → 내림차순)
	// measure_sort("Custom sort (even-asc, odd-desc)", data, ccustom_sort);

	// // 4. 구조체 정렬 테스트
	// sort_employees();

	// {
	// 	std::string aaa = "Lorem ipsum dolor sit amet consectetur adipiscing elit. Blandit quis suspendisse aliquet nisi sodales consequat magna. Sem placerat in id cursus mi pretium tellus. Finibus facilisis dapibus etiam interdum tortor ligula congue. Sed diam urna tempor pulvinar vivamus fringilla lacus. Porta elementum a enim euismod quam justo lectus. Nisl malesuada lacinia integer nunc posuere ut hendrerit. Imperdiet mollis nullam volutpat porttitor ullamcorper rutrum gravida. Ad litora torquent per conubia nostra inceptos himenaeos. Ornare sagittis vehicula praesent dui felis venenatis ultrices. Dis parturient montes nascetur ridiculus mus donec rhoncus. Potenti ultricies habitant morbi senectus netus suscipit auctor. Maximus eget fermentum odio phasellus non purus est. Platea dictumst lorem ipsum dolor sit amet consectetur. Dictum risus blandit quis suspendisse aliquet nisi sodales. Vitae pellentesque sem placerat in id cursus mi. Luctus nibh finibus facilisis dapibus etiam interdum tortor. Eu aenean sed diam urna tempor pulvinar vivamus. Tincidunt nam porta elementum a enim euismod quam. Iaculis massa nisl malesuada lacinia integer nunc posuere. Velit aliquam imperdiet mollis nullam volutpat porttitor ullamcorper. Taciti sociosqu ad litora torquent per conubia nostra. Primis vulputate ornare sagittis vehicula praesent dui felis. Et magnis dis parturient montes nascetur ridiculus mus. Accumsan maecenas potenti ultricies habitant morbi senectus netus. Mattis scelerisque maximus eget fermentum odio phasellus non. Hac habitasse platea dictumst lorem ipsum dolor sit. Vestibulum fusce dictum risus blandit quis suspendisse aliquet. Ex sapien vitae pellentesque sem placerat in id. Neque at luctus nibh finibus facilisis dapibus etiam. Tempus leo eu aenean sed diam urna tempor. Viverra ac tincidunt nam porta elementum a enim. Bibendum egestas iaculis massa nisl malesuada lacinia integer. Arcu dignissim velit aliquam imperdiet mollis nullam volutpat. Class aptent taciti sociosqu ad litora torquent per. Turpis fames primis vulputate ornare sagittis vehicula praesent. Natoque penatibus et magnis dis parturient montes nascetur. Feugiat tristique accumsan maecenas potenti ultricies habitant morbi. Nulla molestie mattis scelerisque maximus eget fermentum odio. Cubilia curae hac habitasse platea dictumst lorem ipsum. Mauris pharetra vestibulum fusce dictum risus blandit quis. Quisque faucibus ex sapien vitae pellentesque sem placerat. Ante condimentum neque at luctus nibh finibus facilisis. Duis convallis tempus leo eu aenean sed diam. Sollicitudin erat viverra ac tincidunt nam porta elementum. Nec metus bibendum egestas iaculis massa nisl malesuada. Commodo augue arcu dignissim velit aliquam imperdiet mollis. Semper vel class aptent taciti sociosqu ad litora. Cras eleifend turpis fames primis vulputate ornare sagittis. Orci varius natoque penatibus et magnis dis parturient. Proin libero feugiat tristique accumsan maecenas potenti ultricies. Eros lobortis nulla molestie mattis scelerisque maximus eget. Curabitur facilisi cubilia curae hac habitasse platea dictumst. Efficitur laoreet mauris pharetra vestibulum fusce dictum risus. Adipiscing elit quisque faucibus ex sapien vitae pellentesque. Consequat magna ante condimentum neque at luctus nibh. Pretium tellus duis convallis tempus leo eu aenean. Ligula congue sollicitudin erat viverra ac tincidunt nam. Fringilla lacus nec metus bibendum egestas iaculis massa. Justo lectus commodo augue arcu dignissim velit aliquam. Ut hendrerit semper vel class aptent taciti sociosqu. Rutrum gravida cras eleifend turpis fames primis vulputate. Inceptos himenaeos orci varius natoque penatibus et magnis. Venenatis ultrices proin libero feugiat tristique accumsan maecenas. Donec rhoncus eros lobortis nulla";
	// 	std::string bbb = std::string("Lorem ipsum dolor sit amet consectetur adipiscing elit. Blandit quis suspendisse aliquet nisi sodales consequat magna. Sem placerat in id cursus mi pretium tellus. Finibus facilisis dapibus etiam interdum tortor ligula congue. Sed diam urna tempor pulvinar vivamus fringilla lacus. Porta elementum a enim euismod quam justo lectus. Nisl malesuada lacinia integer nunc posuere ut hendrerit. Imperdiet mollis nullam volutpat porttitor ullamcorper rutrum gravida. Ad litora torquent per conubia nostra inceptos himenaeos. Ornare sagittis vehicula praesent dui felis venenatis ultrices. Dis parturient montes nascetur ridiculus mus donec rhoncus. Potenti ultricies habitant morbi senectus netus suscipit auctor. Maximus eget fermentum odio phasellus non purus est. Platea dictumst lorem ipsum dolor sit amet consectetur. Dictum risus blandit quis suspendisse aliquet nisi sodales. Vitae pellentesque sem placerat in id cursus mi. Luctus nibh finibus facilisis dapibus etiam interdum tortor. Eu aenean sed diam urna tempor pulvinar vivamus. Tincidunt nam porta elementum a enim euismod quam. Iaculis massa nisl malesuada lacinia integer nunc posuere. Velit aliquam imperdiet mollis nullam volutpat porttitor ullamcorper. Taciti sociosqu ad litora torquent per conubia nostra. Primis vulputate ornare sagittis vehicula praesent dui felis. Et magnis dis parturient montes nascetur ridiculus mus. Accumsan maecenas potenti ultricies habitant morbi senectus netus. Mattis scelerisque maximus eget fermentum odio phasellus non. Hac habitasse platea dictumst lorem ipsum dolor sit. Vestibulum fusce dictum risus blandit quis suspendisse aliquet. Ex sapien vitae pellentesque sem placerat in id. Neque at luctus nibh finibus facilisis dapibus etiam. Tempus leo eu aenean sed diam urna tempor. Viverra ac tincidunt nam porta elementum a enim. Bibendum egestas iaculis massa nisl malesuada lacinia integer. Arcu dignissim velit aliquam imperdiet mollis nullam volutpat. Class aptent taciti sociosqu ad litora torquent per. Turpis fames primis vulputate ornare sagittis vehicula praesent. Natoque penatibus et magnis dis parturient montes nascetur. Feugiat tristique accumsan maecenas potenti ultricies habitant morbi. Nulla molestie mattis scelerisque maximus eget fermentum odio. Cubilia curae hac habitasse platea dictumst lorem ipsum. Mauris pharetra vestibulum fusce dictum risus blandit quis. Quisque faucibus ex sapien vitae pellentesque sem placerat. Ante condimentum neque at luctus nibh finibus facilisis. Duis convallis tempus leo eu aenean sed diam. Sollicitudin erat viverra ac tincidunt nam porta elementum. Nec metus bibendum egestas iaculis massa nisl malesuada. Commodo augue arcu dignissim velit aliquam imperdiet mollis. Semper vel class aptent taciti sociosqu ad litora. Cras eleifend turpis fames primis vulputate ornare sagittis. Orci varius natoque penatibus et magnis dis parturient. Proin libero feugiat tristique accumsan maecenas potenti ultricies. Eros lobortis nulla molestie mattis scelerisque maximus eget. Curabitur facilisi cubilia curae hac habitasse platea dictumst. Efficitur laoreet mauris pharetra vestibulum fusce dictum risus. Adipiscing elit quisque faucibus ex sapien vitae pellentesque. Consequat magna ante condimentum neque at luctus nibh. Pretium tellus duis convallis tempus leo eu aenean. Ligula congue sollicitudin erat viverra ac tincidunt nam. Fringilla lacus nec metus bibendum egestas iaculis massa. Justo lectus commodo augue arcu dignissim velit aliquam. Ut hendrerit semper vel class aptent taciti sociosqu. Rutrum gravida cras eleifend turpis fames primis vulputate. Inceptos himenaeos orci varius natoque penatibus et magnis. Venenatis ultrices proin libero feugiat tristique accumsan maecenas. Donec rhoncus eros lobortis nulla");
	// 	std::string* ptr = new std::string("Dynamic");  
	// 	std::cout << aaa << std::endl;
	// 	std::cout << bbb << std::endl;
	// 	std::cout << *ptr << std::endl;
	// 	delete ptr;
	// }

	// {
	// 	std::string input = "apple orange banana";  
	// 	std::istringstream iss(input);  
	// 	std::vector<std::string> tokens;  
	// 	std::string token;  
		
	// 	while (iss >> token) {  
	// 		tokens.push_back(token);  
	// 	}  
		
	// 	for (const auto& t : tokens) {  
	// 		std::cout << t << std::endl;  
    // 	}
	// }

	std::cout << "Hello World!" << std::endl; 
	return 0;
}
}
