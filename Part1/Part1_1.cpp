#include<iostream> // Provides std::cout, std::cerr and basic I/O streams
#include<fstream>  // Provides file stream classes: std::ifstream, std::ofstream
#include<vector>   // Provides std::vector container template
#include<string>   // Provides std::string
#include<chrono>   // Provides high-resolution clock for timing
#include<algorithm>// Provides std::sort and other algorithms

using namespace std; // Use the standard namespace for convenience

// -----------------------------------------------------------------------------
// Detailed, line-by-line comments follow. The code itself is unchanged.
// Each comment explains purpose and key behaviors so a new reader can follow.
// -----------------------------------------------------------------------------

// Struct to hold the relevant fields for Part 1
// - `date`: CSV date string for display and potential parsing elsewhere
// - `cumulative`: numeric key used by the sorting algorithms
struct TradeRecord{string date;long long cumulative;};

// Function to correctly parse a CSV line, handling commas inside quotes
// Input: a single CSV line as `line`.
// Output: a vector of strings, one per CSV cell.
// Behavior:
//  - Walks each character in the line
//  - Toggles `inQuotes` when a double-quote is seen
//  - Treats commas outside quotes as separators, commas inside quotes as data
vector<string> parseCSVLine(const string& line){
    vector<string> result; // Will hold the parsed cells
    string cell; // Buffer for the current cell's characters
    bool inQuotes=false; // Are we inside a quoted cell right now?

    // Iterate over every character in the input line
    for(char c:line){
        if(c=='"'){
            // Flip the inQuotes flag whenever a quote is encountered.
            // This simple approach assumes quotes are used only to enclose cells.
            inQuotes=!inQuotes;
        } else if(c==','&&!inQuotes){
            // If we encounter a comma and are NOT inside quotes, the current
            // cell ends here. Push it to the result vector and start a new one.
            result.push_back(cell);
            cell.clear();
        } else {
            // Any other character (or comma inside quotes) becomes part of
            // the current cell's content.
            cell+=c;
        }
    }

    // After the loop, the final cell remains in `cell` and must be pushed.
    result.push_back(cell);
    return result;
}

// 1. MERGE SORT IMPLEMENTATION
// The merge operation merges two sorted subranges into one sorted range.
// Parameters:
// - `arr`: the full array holding the ranges
// - `left`, `mid`, `right`: inclusive indices defining the two ranges
void merge(vector<TradeRecord>& arr,int left,int mid,int right){
    int n1=mid-left+1; // Number of elements in the left subarray
    int n2=right-mid; // Number of elements in the right subarray

    // Allocate temporary vectors to hold the two halves
    vector<TradeRecord> L(n1),R(n2);
    for(int i=0;i<n1;i++) L[i]=arr[left+i]; // Copy left half
    for(int j=0;j<n2;j++) R[j]=arr[mid+1+j]; // Copy right half

    int i=0,j=0,k=left; // i -> index for L, j -> index for R, k -> write position

    // Merge until one side is exhausted
    while(i<n1&&j<n2){
        // Compare the `cumulative` keys and copy the smaller (stable behavior
        // is achieved by using <= so left items tie-break left ones first).
        if(L[i].cumulative<=R[j].cumulative){
            arr[k++]=L[i++];
        } else {
            arr[k++]=R[j++];
        }
    }

    // If left side still has elements, copy them
    while(i<n1) arr[k++]=L[i++];

    // If right side still has elements, copy them
    while(j<n2) arr[k++]=R[j++];
}

// Recursive Merge Sort driver
// - Sorts the subarray arr[left..right] in-place
void mergeSort(vector<TradeRecord>& arr,int left,int right){
    if(left>=right) return; // Base case: zero or one element
    int mid=left+(right-left)/2; // Middle index (avoids overflow)
    mergeSort(arr,left,mid); // Sort left half recursively
    mergeSort(arr,mid+1,right); // Sort right half recursively
    merge(arr,left,mid,right); // Merge the two sorted halves
}

// 2. COUNTING SORT IMPLEMENTATION
// Counting sort is effective when the key range (max-min) is not huge.
// This function sorts the vector in ascending order by `cumulative`.
// If the integer range is too big to allocate safely, it falls back to std::sort.
void countingSort(vector<TradeRecord>& arr){
    if(arr.empty()) return; // Nothing to sort for empty input

    // Initialize min/max using the first element
    long long maxVal=arr[0].cumulative;
    long long minVal=arr[0].cumulative;

    // Find the real min and max across all elements
    for(const auto& rec:arr){
        if(rec.cumulative>maxVal) maxVal=rec.cumulative;
        if(rec.cumulative<minVal) minVal=rec.cumulative;
    }

    // Compute the required counting array size (inclusive range)
    long long range=maxVal-minVal+1;

    // Practical safety guard: if the range is enormous, avoid allocating
    // a giant vector which could crash the program or exhaust memory.
    if(range>100000000){ // 100 million threshold (tunable)
        cout<<"   [Warning] Range is too large ("<<range<<") for pure Counting Sort to fit in memory."<<endl;
        cout<<"   [Warning] Falling back to standard sort to prevent crash."<<endl;
        // Use std::sort fallback: O(n log n) but safe in memory usage
        sort(arr.begin(),arr.end(),[](const TradeRecord&a,const TradeRecord&b){
            return a.cumulative<b.cumulative;
        });
        return;
    }

    // Prepare counting and output arrays
    vector<long long> count(range,0); // count[i] -> frequency of value (i+minVal)
    vector<TradeRecord> output(arr.size()); // temporary output buffer

    // Count the occurrences of each key
    for(const auto& rec:arr){
        count[rec.cumulative-minVal]++;
    }

    // Transform counts into cumulative counts which represent positions
    for(size_t i=1;i<count.size();i++){
        count[i]+=count[i-1];
    }

    // Build the output array by placing elements in correct positions.
    // Iterate from right to left to keep the sort stable for equal keys.
    for(int i=(int)arr.size()-1;i>=0;i--){
        long long keyIndex=arr[i].cumulative-minVal; // shifted index
        long long pos=count[keyIndex]-1; // position in output
        output[pos]=arr[i]; // place element
        count[keyIndex]--; // decrement count
    }

    // Copy sorted data back into original array
    for(size_t i=0;i<arr.size();i++){
        arr[i]=output[i];
    }
}

int main(){
    // The CSV filename expected to be in the same directory as the executable
    string filename="effects-of-covid-19-on-trade-at-15-december-2021-provisional.csv";
    ifstream file(filename); // Open file for reading

    // If opening failed, report to stderr and exit with non-zero status
    if(!file.is_open()){
        cerr<<"Error: Could not open the file "<<filename<<". Make sure it's in the same folder!"<<endl;
        return 1;
    }

    vector<TradeRecord> dataset; // Will hold all parsed records
    string line; // Temporary buffer for each input line

    // Skip the first line which usually contains column headers
    getline(file,line);

    cout<<"Reading data from CSV..."<<endl;
    // Read the file line by line until EOF
    while(getline(file,line)){
        if(line.empty()) continue; // Skip empty lines silently

        // Parse the CSV line into fields
        vector<string> row=parseCSVLine(line);

        // Ensure expected number of columns (we need at least 10 to read index 9)
        if(row.size()>=10){
            TradeRecord record; // Create a record to populate
            record.date=row[2]; // Column 3 contains the date string

            // Parse the cumulative numeric field from column 10 (index 9)
            try{
                record.cumulative=stoll(row[9]); // Convert string->long long
            } catch(...){
                // If parsing fails (empty string or invalid), use zero as fallback
                record.cumulative=0;
            }

            // Append the populated record into dataset
            dataset.push_back(record);
        }
    }
    file.close(); // Close the file as soon as parsing is complete

    cout<<"Successfully loaded "<<dataset.size()<<" records.\n"<<endl;

    // Create copies so both algorithms sort the exact same unsorted data
    vector<TradeRecord> datasetForMerge=dataset; // copy for merge sort
    vector<TradeRecord> datasetForCounting=dataset; // copy for counting sort

    // --- TIMING MERGE SORT ---
    cout<<"Starting Merge Sort..."<<endl;
    auto startMerge=chrono::high_resolution_clock::now();

    // Run merge sort on the dedicated copy. Cast indices to int for safety.
    mergeSort(datasetForMerge,0,(int)datasetForMerge.size()-1);

    auto endMerge=chrono::high_resolution_clock::now();
    chrono::duration<double,milli> durationMerge=endMerge-startMerge;
    cout<<"--> Merge Sort finished in "<<durationMerge.count()<<" ms.\n"<<endl;


    // TIMING COUNTING SORT
    cout<<"Starting Counting Sort..."<<endl;
    auto startCounting=chrono::high_resolution_clock::now();

    // Counting sort may fall back to std::sort internally if range is large
    countingSort(datasetForCounting);

    auto endCounting=chrono::high_resolution_clock::now();
    chrono::duration<double,milli> durationCounting=endCounting-startCounting;
    cout<<"--> Counting Sort finished in "<<durationCounting.count()<<" ms.\n"<<endl;

    // Print first 5 entries from each sorted result as a basic correctness check.
    cout<<"Top 5 sorted records (Merge Sort check):"<<endl;
    for(int i=0;i<5&&i<(int)datasetForMerge.size();i++){
        cout<<"Date: "<<datasetForMerge[i].date<<" | Cumulative: "<<datasetForMerge[i].cumulative<<endl;
    }

    cout<<"Top 5 sorted records (Counting Sort check):"<<endl;
    for(int i=0;i<5&&i<(int)datasetForCounting.size();i++){
        cout<<"Date: "<<datasetForCounting[i].date<<" | Cumulative: "<<datasetForCounting[i].cumulative<<endl;
    }

    return 0;
}