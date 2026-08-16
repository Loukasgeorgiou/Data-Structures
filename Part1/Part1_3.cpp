// Include input/output stream library for console I/O (cout, cerr)
#include<iostream>
// Include file stream library to read the CSV file
#include<fstream>
// Include the vector container used to hold records
#include<vector>
// Include string utilities for parsing and storage
#include<string>
// Include chrono for high-resolution timing measurements
#include<chrono>
// Include algorithm for sorting the dataset
#include<algorithm>

// Use the standard namespace to avoid `std::` prefixes in this instructional file
using namespace std;

// -----------------------------------------------------------------------------
// Overview:
// This program compares two search algorithms on a dataset indexed by a
// numeric key derived from dates. The comparisons measure steps and timings.
//
// High-level flow:
// 1. Read CSV lines into `TradeRecord` structures.
// 2. Convert date strings to a numeric day-count key using `dateToDays`.
// 3. Sort the records by that numeric key.
// 4. Search for a sample date using Binary Search and Interpolation Search.
// 5. Print index, step counts and timings for each search.
// -----------------------------------------------------------------------------

// Struct to represent a single trade record parsed from the CSV file.
// Fields:
// - `date`: original date string from the CSV (format DD/MM/YYYY)
// - `key`: numeric key derived from `date` (days since 01/01/2015)
// - `value`: numeric value parsed from a CSV column
// - `cumulative`: numeric cumulative value parsed from a CSV column
struct TradeRecord{
    string date; // original date text (DD/MM/YYYY)
    long long key; // total days since 01/01/2015 (computed)
    long long value; // parsed numeric field (from CSV column)
    long long cumulative; // parsed cumulative field (from CSV column)
};

// Convert a date string "DD/MM/YYYY" into the number of days since
// 01/01/2015. Returns -1 for strings that are too short to be valid dates.
long long dateToDays(const string& dateStr){
    // Quick sanity check: a valid DD/MM/YYYY string should be at least 10 chars
    if(dateStr.length()<10) return -1;

    // Extract day, month and year substrings and convert to integers.
    int d=stoi(dateStr.substr(0,2)); // day part: characters 0-1
    int m=stoi(dateStr.substr(3,2)); // month part: characters 3-4
    int y=stoi(dateStr.substr(6,4)); // year part: characters 6-9

    // Table of cumulative days before each month for non-leap years.
    // Index corresponds to month number; element 0 unused, element 1 = January.
    static const int daysBeforeMonth[] = {0,0,31,59,90,120,151,181,212,243,273,304,334};

    // Start with a base number of days contributed by whole years since 2015.
    long long totalDays=(y-2015)*365;

    // Account for leap days between 2015 (inclusive) and the year before `y`.
    for(int yr=2015;yr<y;++yr){
        // Leap year if divisible by 4 and not by 100, unless divisible by 400.
        if((yr%4==0&&yr%100!=0)||(yr%400==0)) totalDays++;
    }

    // Add days contributed by months passed in the target year plus days in month.
    totalDays+=daysBeforeMonth[m]+(d-1);

    // If the date is after February in a leap year, add the leap day for that year.
    if(m>2&&((y%4==0&&y%100!=0)||(y%400==0))) totalDays++;

    // Return total days offset since 01/01/2015.
    return totalDays;
}

// Parse a single CSV line into its comma-separated cells while handling
// quoted cells that may contain commas. Returns a vector of cell strings.
vector<string> parseCSVLine(const string& line){
    vector<string> result; // container for parsed cells
    string cell; // accumulator for the current cell's characters
    bool inQuotes=false; // whether the parser is currently inside quotes

    // Iterate each character and build cells respecting quoted fields.
    for(char c:line){
        if(c=='"'){
            // Toggle quoted state when encountering a double quote
            inQuotes=!inQuotes;
        } else if(c==','&&!inQuotes){
            // If we hit a comma outside quotes, the current cell ends here
            result.push_back(cell);
            cell.clear();
        } else {
            // Normal character: append to the current cell
            cell+=c;
        }
    }

    // Push the final cell after the loop
    result.push_back(cell);
    return result;
}

// Binary Search implementation over `TradeRecord` array by `key`.
// - `arr`: sorted array of TradeRecord (ascending by `key`)
// - `target`: the numeric key to search for
// - `steps`: output parameter counting how many iterations were performed
// Returns the index of a matching element or -1 if not found.
int binarySearch(const vector<TradeRecord>& arr,long long target,long long& steps){
    int low=0; // lower bound index
    int high=(int)arr.size()-1; // upper bound index
    steps=0; // reset step counter

    // Standard binary search loop: shrink the search interval by half each step
    while(low<=high){
        steps++; // increment iteration counter for analysis
        int mid=low+(high-low)/2; // avoid potential overflow for large indices

        // Check the middle element for equality
        if(arr[mid].key==target) return mid;

        // Decide which subinterval to keep based on comparison
        if(arr[mid].key<target) low=mid+1;
        else high=mid-1;
    }

    // Not found: return -1
    return -1;
}

// Interpolation Search implementation over `TradeRecord` array by `key`.
// Interpolation search estimates the likely position of `target` assuming
// keys are (roughly) uniformly distributed. It can be faster than binary
// search for such distributions.
// - `arr`: sorted array of TradeRecord
// - `target`: numeric key to search for
// - `steps`: output parameter counting iterations
// Returns the index of a matching element or -1 if not found.
int interpolationSearch(const vector<TradeRecord>& arr,long long target,long long& steps){
    int low=0; // current lower bound
    int high=(int)arr.size()-1; // current upper bound
    steps=0; // reset counter

    // Continue while the search interval is valid and the target is within bounds
    while(low<=high&&target>=arr[low].key&&target<=arr[high].key){
        steps++; // count this iteration

        // If the keys at bounds are equal, we cannot interpolate; check equality
        if(arr[low].key==arr[high].key){
            if(arr[low].key==target) return low; // both bounds match target
            break; // keys are all equal but do not match target -> not found
        }

        // Compute ratio of how far `target` lies between low and high keys
        double ratio=(double)(target-arr[low].key)/(arr[high].key-arr[low].key);

        // Estimate position using the ratio scaled to index range
        int pos=low+(int)(ratio*(high-low));

        // Safety: ensure estimated position falls inside current interval
        if(pos<low||pos>high) break;

        // Check the estimated position
        if(arr[pos].key==target) return pos;
        if(arr[pos].key<target) low=pos+1; // move lower bound up
        else high=pos-1; // move upper bound down
    }

    // Target not found in the array
    return -1;
}

// Program entry point
int main(){
    // Name of the CSV file expected in the working directory
    string filename="effects-of-covid-19-on-trade-at-15-december-2021-provisional.csv";

    // Open the CSV file for reading
    ifstream file(filename);

    // Check that the file was opened successfully; otherwise print error and exit
    if(!file.is_open()){
        cerr<<"Error: File "<<filename<<" could not be opened."<<endl;
        return 1; // non-zero return indicates failure to the OS
    }

    vector<TradeRecord> dataset; // container for parsed records
    string line; // temporary storage for each CSV line

    // Read and discard the header line (first line contains column names)
    getline(file,line); // Skip header

    // Read each remaining line from the file
    while(getline(file,line)){
        // Ignore empty lines that may appear in the file
        if(line.empty()) continue;

        // Split the line into cells handling quoted commas
        vector<string> row=parseCSVLine(line);

        // Only proceed if we have at least the expected number of columns
        if(row.size()>=10){
            TradeRecord rec; // local record to populate
            rec.date=row[2]; // date is stored in column index 2
            rec.key=dateToDays(rec.date); // compute numeric key from date

            // Parse numeric fields with exception safety; default to 0 on parse errors
            try{ rec.value=stoll(row[8]); } catch(...){ rec.value=0; }
            try{ rec.cumulative=stoll(row[9]); } catch(...){ rec.cumulative=0; }

            // Add the populated record to the dataset
            dataset.push_back(rec);
        }
    }

    // Close the file now that parsing is complete
    file.close();

    // Sort the dataset in ascending order by the numeric `key` (days since 01/01/2015)
    sort(dataset.begin(),dataset.end(),[](const TradeRecord& a,const TradeRecord& b){ return a.key<b.key; });

    // Choose an example input date to search for in the dataset
    string inputDate="15/06/2018";
    long long targetKey=dateToDays(inputDate); // convert example date to numeric key

    long long stepsBS=0,stepsIS=0; // counters for the two search algorithms

    // Time Binary Search: capture start time, run search, capture end time
    auto t1=chrono::high_resolution_clock::now();
    int idxBS=binarySearch(dataset,targetKey,stepsBS);
    auto t2=chrono::high_resolution_clock::now();

    // Run Interpolation Search and capture time after it completes
    int idxIS=interpolationSearch(dataset,targetKey,stepsIS);
    auto t3=chrono::high_resolution_clock::now();

    // Print results: index found (-1 if not found), number of steps, and nanosecond timings
    cout<<"--- TASK 3 RESULTS ("<<inputDate<<") ---"<<endl;
    cout<<"Binary Search:        Index = "<<idxBS<<" | Steps = "<<stepsBS
        <<" | Time = "<<chrono::duration_cast<chrono::nanoseconds>(t2-t1).count()<<" ns"<<endl;
    cout<<"Interpolation Search: Index = "<<idxIS<<" | Steps = "<<stepsIS
        <<" | Time = "<<chrono::duration_cast<chrono::nanoseconds>(t3-t2).count()<<" ns"<<endl;

    // Return success
    return 0;
}