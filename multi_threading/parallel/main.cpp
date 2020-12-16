#include <string>
#include <cstring>
#include <fstream>
#include <vector>
#include <utility>
#include <stdexcept>
#include <sstream>
#include <iostream>
#include "pthread.h"

#define NUM_THREADS 4

using namespace std;

string datasetDirName;
int target_Num = 0;
vector<pair<string, vector<double>>> train[NUM_THREADS];

vector<pair<string, vector<double>>> read_csv(string filename, int* colNum){
    // Reads a CSV file into a vector of <string, vector<int>> pairs where
    // each pair represents <column name, column values>

    // Create a vector of <string, int vector> pairs to store the result
    vector<pair<string, vector<double>>> result;

    // Create an input filestream
    ifstream myFile(filename);

    // Make sure the file is open
    if(!myFile.is_open()) throw runtime_error("Could not open file " + filename);

    // Helper vars
    string line, colname;
    double val;
    int colNum_H = 0;

    // Read the column names
    if(myFile.good())
    {
        // Extract the first line in the file
        getline(myFile, line);

        // Create a stringstream from line
        stringstream ss(line);

        // Extract each column name
        while(getline(ss, colname, ',')){
            colNum_H ++;
            // Initialize and add <colname, int vector> pairs to result
            result.push_back({colname, vector<double> {}});
        }
    }

    // Read data, line by line
    while(getline(myFile, line))
    {
        // Create a stringstream of the current line
        stringstream ss(line);

        // Keep track of the current column index
        int colIdx = 0;

        // Extract each integer
        while(ss >> val){

            // Add the current integer to the 'colIdx' column's values vector
            result.at(colIdx).second.push_back(val);

            // If the next token is a comma, ignore it and move on
            if(ss.peek() == ',') ss.ignore();

            // Increment the column index
            colIdx++;
        }
    }

    // Close file
    myFile.close();
    *colNum = colNum_H;
    return result;
}

void* read_csv_thread(void* indexPtr){
    // Reads a CSV file into a vector of <string, vector<int>> pairs where
    // each pair represents <column name, column values>

    // compute filename
    long index = (long)indexPtr;
    string filename;
    stringstream ss;
    ss << datasetDirName << "train_" << index << ".csv";
    ss >> filename;

    // Create a vector of <string, int vector> pairs to store the result
    vector<pair<string, vector<double>>> result;

    // Create an input filestream
    ifstream myFile(filename);

    // Make sure the file is open
    if(!myFile.is_open()){
            stringstream ss;
            string message;
            ss << "Could not open file " << filename;
            ss >> message;
            throw runtime_error(message);
    }
    // Helper vars
    string line, colname;
    double val;
    int colNum_H = 0;

    // Read the column names
    if(myFile.good())
    {
        // Extract the first line in the file
        getline(myFile, line);

        // Create a stringstream from line
        stringstream ss(line);

        // Extract each column name
        while(getline(ss, colname, ',')){
            colNum_H ++;
            // Initialize and add <colname, int vector> pairs to result
            result.push_back({colname, vector<double> {}});
        }
    }

    // Read data, line by line
    while(getline(myFile, line))
    {
        // Create a stringstream of the current line
        stringstream ss(line);

        // Keep track of the current column index
        int colIdx = 0;

        // Extract each integer
        while(ss >> val){

            // Add the current integer to the 'colIdx' column's values vector
            result.at(colIdx).second.push_back(val);

            // If the next token is a comma, ignore it and move on
            if(ss.peek() == ',') ss.ignore();

            // Increment the column index
            colIdx++;
        }
    }

    // Close file
    myFile.close();
    target_Num = colNum_H;

    train[index] = result;
    return NULL;
}

vector<pair<string, vector<double>>> read_csv_parallel(string dirName){
    vector<pair<string, vector<double>>> result;

    pthread_t tid_0, tid_1, tid_2, tid_3;

    pthread_create(&tid_0, NULL, &read_csv_thread, (void *)0);
    pthread_create(&tid_1, NULL, &read_csv_thread, (void *)1);
    pthread_create(&tid_2, NULL, &read_csv_thread, (void *)2);
    pthread_create(&tid_3, NULL, &read_csv_thread, (void *)3);

    pthread_join(tid_0, NULL);
    pthread_join(tid_1, NULL);
    pthread_join(tid_2, NULL);
    pthread_join(tid_3, NULL);

    for(int i = 0; i < NUM_THREADS; i++){
        result.insert(result.end(), train[i].begin(), train[i].end());
    }
    return result;
}

double findMin(vector<pair<string, vector<double>>> train, int column_Num) {
    int minVal = INT8_MAX;
    for (int i = 0; i < train.at(column_Num).second.size(); i++) {
        if (minVal > train.at(column_Num).second[i]) {
            minVal = train.at(column_Num).second[i];
        }
    }
    return minVal;
}

double findMax(vector<pair<string, vector<double>>> train, int column_Num) {
    int maxVal = INT8_MIN;
    for (int i = 0; i < train.at(column_Num).second.size(); i++) {
        if (maxVal < train.at(column_Num).second[i]) {
            maxVal = train.at(column_Num).second[i];
        }
    }
    return maxVal;
}

vector<int> predict(vector<pair<string, vector<double>>> train, vector<pair<string, vector<double>>> weights, int column_Num, int target_Num) {
    vector<int> result;
    for (int i = 0; i < train.at(0).second.size(); i++) {
        double point0 = weights.at(column_Num - 1).second[0];
        double point1 = weights.at(column_Num - 1).second[1];
        double point2 = weights.at(column_Num - 1).second[2];
        double point3 = weights.at(column_Num - 1).second[3];
        for (int j = 0; j < target_Num - 1; j++) {
            point0 = point0 + train.at(j).second[i] * weights.at(j).second[0];
            point1 = point1 + train.at(j).second[i] * weights.at(j).second[1];
            point2 = point2 + train.at(j).second[i] * weights.at(j).second[2];
            point3 = point3 + train.at(j).second[i] * weights.at(j).second[3];
        }
        if (point0 >= point1 && point0 >= point2 && point0 >= point3) {
            result.push_back(0);
        } else if (point1 >= point0 && point1 >= point2 && point1 >= point3) {
            result.push_back(1);
        } else if (point2 >= point1 && point2 >= point0 && point2 >= point3) {
            result.push_back(2);
        } else if (point3 >= point1 && point3 >= point2 && point3 >= point0) {
            result.push_back(3);
        }
    }
    return result;
}

double accuracy(vector<pair<string, vector<double>>> train, vector<int> result, int target_Num) {
    int point = 0;
    for (int i = 0; i < train.at(0).second.size(); i++) {
        if (train[target_Num - 1].second[i] == result[i]) {
            point ++;
        }
    }
    return (point /(double) train.at(0).second.size()) * 100;
}

int main(int argc, char *argv[]) {
    int colNum1 = 0;
    if (argc < 2) {
        throw runtime_error("Dataset directory is not specified");
    }
    datasetDirName = argv[1];

    vector<pair<string, vector<double>>> train = read_csv_parallel(datasetDirName);

    vector<pair<string, vector<double>>> weights = read_csv(datasetDirName + "weights.csv", &colNum1);

    // Normalization
    for (int i = 0; i < target_Num - 1; i++) {
        double minVal = findMin(train, i);
        double maxVal = findMax(train, i);
        for (int j = 0; j < train.at(i).second.size(); j++) {
            train[i].second[j] = (train[i].second[j] - minVal) / (maxVal - minVal);
        }
    }

    // Prediction
    vector<int> result = predict(train, weights, colNum1, target_Num);

    // Calculating accuracy
    double accur = accuracy(train, result, target_Num);

    cout << "Accuracy: " << accur << "%";

    return 0;
}