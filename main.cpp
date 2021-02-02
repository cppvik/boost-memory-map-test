#include <boost/iostreams/device/mapped_file.hpp>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <chrono>

using namespace std;

static constexpr uint16_t hashLength = 20;
static constexpr size_t blockSize = 102400;

struct Block {
    uint8_t hash[hashLength];
    bool data[1];
};

void testDataBlocksWrite(boost::iostreams::mapped_file &file) {
    if (!file.is_open())
        return;

    auto numberOfBlocks = 100;
    auto start = chrono::high_resolution_clock::now();
    size_t *currentBlock = reinterpret_cast<size_t *>(file.data());
    for (auto i = 1; i <= numberOfBlocks; i++) {
        *currentBlock = i;
        auto block = reinterpret_cast<Block *>(file.data() + sizeof(size_t) + *currentBlock * (sizeof(Block) + blockSize));
        for (auto j = 0; j < hashLength; j++)
            block->hash[j] = i;
        for (auto j = 0; j < blockSize; j++)
            block->data[j] = i % 2;
    }
    auto end = chrono::high_resolution_clock::now();
    auto elapsed = chrono::duration_cast<chrono::milliseconds>(end - start);
    cout << numberOfBlocks << " data blocks written in " << elapsed.count() << " ms" << endl;
}

void testDataBlocksRead(boost::iostreams::mapped_file &file) {
    if (!file.is_open())
        return;

    auto numberOfBlocks = 100;
    auto start = chrono::high_resolution_clock::now();

    size_t *currentBlock = reinterpret_cast<size_t *>(file.data());
    if (*currentBlock != numberOfBlocks) {
        cout << *currentBlock << " != " << numberOfBlocks << endl;
        return;
    }
    for (auto i = 0; i < numberOfBlocks; i++) {
        auto block = reinterpret_cast<Block *>(file.data() + sizeof(size_t) + i * (sizeof(Block) + blockSize));
        for (auto j = 0; j < hashLength; j++)
            if (block->hash[j] != i) {
                cout << "Wrong block->hash[" << j << "]: " << block->hash[j] << " != " << i << endl;
                break;
            }
        for (auto j = 0; j < blockSize; j++)
            if (block->data[j] != (i % 2)) {
                cout << "Wrong block->data[" << j << "]: " << block->data[j] << " != " << i % 2 << endl;
                break;
            }
    }
    auto end = chrono::high_resolution_clock::now();
    auto elapsed = chrono::duration_cast<chrono::milliseconds>(end - start);
    cout << numberOfBlocks << " data blocks read and checked in " << elapsed.count() << " ms" << endl;
}

void testAlignment(boost::iostreams::mapped_file &file) {
    auto alignment = file.alignment();
    cout << "The operating system's virtual memory allocation granularity: " << alignment << endl;

    chrono::time_point<chrono::high_resolution_clock> start;
    chrono::time_point<chrono::high_resolution_clock> end;
    auto elapsed = chrono::duration_cast<chrono::nanoseconds>(end - start);

    // write test, we use r\w data pointer here
    auto *data = reinterpret_cast<int8_t *>(file.data());
    for (auto i = alignment * 5 - 5; i < alignment * 5 + 5; i++) {
        start = chrono::high_resolution_clock::now();
        data[i] = 0;
        end = chrono::high_resolution_clock::now();
        elapsed = chrono::duration_cast<chrono::nanoseconds>(end - start);
        cout << "Byte #" << i << " rewritten in " << elapsed.count() << " ns" << endl;
    }

    // read test, we use r/o data pointer here
    auto *constData = reinterpret_cast<const int8_t *>(file.const_data());
    int8_t value = 0;
    for (auto i = alignment * 10 - 5; i < alignment * 10 + 5; i++) {
        start = chrono::high_resolution_clock::now();
        value = constData[i];
        end = chrono::high_resolution_clock::now();
        elapsed = chrono::duration_cast<chrono::nanoseconds>(end - start);
        cout << "Byte #" << i << " read in " << elapsed.count() << " ns" << endl;
    }
}

// Read last bytes
void testWriteData(boost::iostreams::mapped_file &file) {
    auto *data = reinterpret_cast<int8_t *>(file.data());
    auto start = chrono::high_resolution_clock::now();
    for (auto i = 0; i < file.size(); i++)
        data[i] = 0;
    auto end = chrono::high_resolution_clock::now();
    auto elapsed = chrono::duration_cast<chrono::milliseconds>(end - start);
    cout << file.size() <<  " bytes changed in " << elapsed.count() << "ms" << endl;
}

// Erase all data with zeroes
void testReadData(boost::iostreams::mapped_file &file) {
    int numberOfElements = 100;
    auto *data = reinterpret_cast<int8_t *>(file.data());
    for (auto i = 0; i < numberOfElements; i++)
        cout << int{data[/*file.size() - */i]} << " ";
    cout << endl;
}


int main(int argc, char **argv) {
    boost::iostreams::mapped_file file;
    boost::iostreams::mapped_file_params params;
    params.path = "filename.raw";
    params.flags = boost::iostreams::mapped_file::readwrite;

    // Test file open time
    auto start = chrono::high_resolution_clock::now();
    try {
        file.open(params);
    }
    catch (const ios_base::failure &e) {
        cerr << "Error opening file: " << e.what() << endl;
        params.new_file_size = 1024 * 1024 * 10;
        cout << "Creating new file of size " << params.new_file_size << endl;
        file.open(params);
    }

    if (file.is_open()) {
        cout << "File of size " << file.size() << " opened in ";
        auto end = chrono::high_resolution_clock::now();
        auto elapsed = chrono::duration_cast<chrono::milliseconds>(end - start);
        cout << elapsed.count() << "ms" << endl;

        // testReadData(file);
        // testWriteData(file);
        // testAlignment(file);
        testDataBlocksWrite(file);
        testDataBlocksRead(file);

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
        file.close();
        elapsed = chrono::duration_cast<chrono::milliseconds>(end - start);
        cout << "File reopened in " << elapsed.count() << "ms" << endl;
    }
    else {
        cout << "could not map file " << params.path << endl;
    }
}
