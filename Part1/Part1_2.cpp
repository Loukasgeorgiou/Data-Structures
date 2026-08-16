#include<iostream>
#include<fstream>
#include<vector>
#include<string>
#include<chrono>

// Use the C++ standard library namespace to avoid std:: qualification below
using namespace std;

// -----------------------------------------------------------------------------
// Overview:
// This program compares Heap Sort and Quick Sort on the `value` field from a
// CSV of trade records. The program reads the CSV, parses each row, times
// both algorithms on identical copies of the data, and prints a small sample
// of sorted outputs for verification.
//
// Notes on structure:
// - `TradeRecord` stores fields extracted from each CSV row.
// - `parseCSVLine` is a resilient CSV parser that handles quoted commas.
// - `heapify`/`heapSort` implement in-place heap sort (max-heap).
// - `partition`/`quickSort` implement Hoare-style quicksort using a middle
//   pivot to reduce worst-case behavior on already sorted arrays.
// -----------------------------------------------------------------------------

// Struct to hold the relevant fields for Part 1
// Each line in the CSV will populate one TradeRecord instance.
// - `date` stores the human-readable date string from the CSV.
// - `value` stores the numeric field used as the sort key by the algorithms.
struct TradeRecord{string date;long long value;};

// Function to safely read the CSV (ignores commas inside quotes)
// parseCSVLine: split a CSV line into fields handling quoted commas.
// - `line`: input CSV line
// - returns: vector of fields as strings
vector<string> parseCSVLine(const string& line){
    vector<string> result; // will collect parsed fields
    string cell; // buffer for current field characters
    bool inQuotes=false; // state: are we inside a quoted field?

    // Iterate through each character in the input line
    for(char c:line){
        // If we see a double-quote, toggle the inQuotes flag
        if(c=='"'){
            inQuotes=!inQuotes;
        }
        // If we see a comma outside quotes, that marks the end of a cell
        else if(c==','&&!inQuotes){
            result.push_back(cell); // push completed cell
            cell.clear(); // reset buffer for next cell
        } else {
            // Any other character (or comma inside quotes) is appended
            cell+=c;
        }
    }

    // Push the final cell after the loop finishes
    result.push_back(cell);
    return result;
}

// 1. HEAP SORT IMPLEMENTATION
// heapify: ensure subtree rooted at index i satisfies max-heap property
// Parameters:
// - arr: array representation of heap
// - n: number of elements in heap to consider
// - i: index of root of subtree to heapify
void heapify(vector<TradeRecord>& arr,int n,int i){
    int largest=i; // initialize largest as root
    int left=2*i+1; // left child index (0-based)
    int right=2*i+2; // right child index

    // If left child is within heap and its value is larger, update largest
    if(left<n&&arr[left].value>arr[largest].value) largest=left;

    // If right child is within heap and its value is larger, update largest
    if(right<n&&arr[right].value>arr[largest].value) largest=right;

    // If the largest element is not the root, swap and continue heapifying
    if(largest!=i){
        swap(arr[i],arr[largest]); // move largest to root position
        heapify(arr,n,largest); // recursively heapify the affected subtree
    }
}

// heapSort: perform in-place heap sort using the above heapify function
// Steps:
// 1. Build a max-heap from the array.
// 2. Repeatedly swap the root (max element) with the last element of the
//    heap and reduce the heap size by one, then heapify the root.
void heapSort(vector<TradeRecord>& arr){
    int n=arr.size();

    // Build the max-heap: call heapify on all non-leaf nodes in reverse
    for(int i=n/2-1;i>=0;i--) heapify(arr,n,i);

    // One by one, extract elements from heap and place them at the end
    for(int i=n-1;i>0;i--){
        swap(arr[0],arr[i]); // move current largest to correct position
        heapify(arr,i,0); // heapify reduced heap
    }
}

// 2. QUICK SORT IMPLEMENTATION (Hoare Partition)
// partition: Hoare partition scheme for quicksort
// - Chooses pivot as middle element to reduce chance of worst-case on sorted input
// - Returns index j such that arr[low..j] <= pivot and arr[j+1..high] >= pivot
int partition(vector<TradeRecord>& arr,int low,int high){
    long long pivot=arr[low+(high-low)/2].value; // pivot value
    int i=low-1; // left pointer
    int j=high+1; // right pointer

    while(true){
        // Move left pointer until an element >= pivot is found
        do{i++;}while(arr[i].value<pivot);
        // Move right pointer until an element <= pivot is found
        do{j--;}while(arr[j].value>pivot);

        // If pointers crossed, partitioning is finished
        if(i>=j) return j;
        // Swap elements that are on the wrong side of the pivot
        swap(arr[i],arr[j]);
    }
}

// quickSort: recursive Hoare quicksort implementation
// - sorts the inclusive range arr[low..high]
void quickSort(vector<TradeRecord>& arr,int low,int high){
    if(low<high){
        int pi=partition(arr,low,high); // partition index
        quickSort(arr,low,pi); // sort left part
        quickSort(arr,pi+1,high); // sort right part
    }
}

int main(){
    string filename="effects-of-covid-19-on-trade-at-15-december-2021-provisional.csv";
    ifstream file(filename);

    if(!file.is_open()){
        cerr<<"Error: Could not open the file "<<filename<<". Make sure it's in the same folder!"<<endl;
        return 1;
    }

    // Container to hold all parsed TradeRecord entries
    vector<TradeRecord> dataset;
    // Temporary string to hold each CSV line while reading
    string line;

    // Read and discard the CSV header line (column names)
    getline(file,line);

    // Inform the user parsing is starting
    cout<<"Reading data from CSV..."<<endl;

    // Read file line by line until EOF
    while(getline(file,line)){
        // Skip blank lines silently
        if(line.empty()) continue;

        // Split the CSV line into fields, handling quoted commas
        vector<string> row=parseCSVLine(line);

        // Check that the expected index exists (index 8 is `value`)
        if(row.size()>=9){
            TradeRecord record; // temporary record to populate
            record.date=row[2]; // column 3 is the date string

            // Parse the numeric `value` field; fallback to 0 if invalid
            try{
                record.value=stoll(row[8]);
            } catch(...){
                record.value=0; // default on parse failure
            }

            // Append parsed record to dataset
            dataset.push_back(record);
        }
    }

    // Close the file now that we've finished reading
    file.close();

    // Report how many records we successfully loaded
    cout<<"Successfully loaded "<<dataset.size()<<" records.\n"<<endl;

    // Duplicate dataset so each algorithm operates on identical unsorted input
    vector<TradeRecord> datasetForHeap=dataset;
    vector<TradeRecord> datasetForQuick=dataset;

    // HEAP SORT: measure runtime
    cout<<"Starting Heap Sort..."<<endl;
    auto startHeap=chrono::high_resolution_clock::now();
    heapSort(datasetForHeap); // in-place sort on copy
    auto endHeap=chrono::high_resolution_clock::now();
    chrono::duration<double,milli> durationHeap=endHeap-startHeap;
    cout<<"--> Heap Sort finished in "<<durationHeap.count()<<" ms.\n"<<endl;

    // QUICK SORT: measure runtime
    cout<<"Starting Quick Sort..."<<endl;
    auto startQuick=chrono::high_resolution_clock::now();
    quickSort(datasetForQuick,0,(int)datasetForQuick.size()-1);
    auto endQuick=chrono::high_resolution_clock::now();
    chrono::duration<double,milli> durationQuick=endQuick-startQuick;
    cout<<"--> Quick Sort finished in "<<durationQuick.count()<<" ms.\n"<<endl;

    // Verify by printing the first 5 entries of each sorted array (if present)
    cout<<"Top 5 records (Heap Sort verification):"<<endl;
    for(int i=0;i<5&&i<(int)datasetForHeap.size();i++){
        cout<<"Date: "<<datasetForHeap[i].date<<" | Value: "<<datasetForHeap[i].value<<endl;
    }

    cout<<"Top 5 records (Quick Sort verification):"<<endl;
    for(int i=0;i<5&&i<(int)datasetForQuick.size();i++){
        cout<<"Date: "<<datasetForQuick[i].date<<" | Value: "<<datasetForQuick[i].value<<endl;
    }

    return 0;
}