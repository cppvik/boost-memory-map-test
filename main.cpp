#include <boost/iostreams/device/mapped_file.hpp>
#include <iostream>
#include <thread>
#include <chrono>

using namespace std;

int main() {
    int numberOfElements = 10;

    boost::iostreams::mapped_file file;
    boost::iostreams::mapped_file_params params;
    params.path = "filename.raw";
    params.flags = boost::iostreams::mapped_file::readwrite;

    // Test file open time
    auto start = chrono::high_resolution_clock::now();
    file.open(params);
    auto end = chrono::high_resolution_clock::now();
    auto elapsed = chrono::duration_cast<chrono::milliseconds>(end - start);

    if (file.is_open()) {
        cout << "File of size " << file.size() << " opened in " << elapsed.count() << "ms" << endl;
        // Test read last bytes
        auto *data = reinterpret_cast<int8_t *>(file.data());
        for (auto i = 0; i < numberOfElements; i++)
            cout << int{data[file.size() - i]} << " ";
        cout << endl;

        // Erase all data with zeroes
        for (auto i = 0; i < file.size(); i++)
            data[i] = 0;
        cout << endl;

        // Don't forget to unmap
        start = chrono::high_resolution_clock::now();
        file.close();
        end = chrono::high_resolution_clock::now();
        elapsed = chrono::duration_cast<chrono::milliseconds>(end - start);
        cout << "File closed in " << elapsed.count() << "ms" << endl;

        // Test file reopen time
        start = chrono::high_resolution_clock::now();
        file.open(params);
        end = chrono::high_resolution_clock::now();
        elapsed = chrono::duration_cast<chrono::milliseconds>(end - start);
        cout << "File reopened in " << elapsed.count() << "ms" << endl;
    }
    else {
        cout << "could not map file " << params.path << endl;
    }
}