#include<iostream>
#include<fstream>
#include<vector>
#include<string>
#include<chrono>
#include<algorithm>
#include<cmath>

using namespace std;

// -----------------------------------------------------------------------------
// Detailed explanatory comments for Part1_4.cpp
// This file implements two interpolation-based search algorithms (BIS and BIS*)
// and demonstrates them on a dataset loaded from CSV. The code below adds
// comments that explain almost every line and function so a reader unfamiliar
// with the code can follow along. The behavior/logic of the program is not
// changed — only explanatory comments are added.
// -----------------------------------------------------------------------------

// Data structure to hold parsed CSV fields and a derived numeric key.
// - `date` holds the original date string from the CSV
// - `key` stores a numeric representation of the date (days since 01/01/2015)
// - `value` and `cumulative` store other numeric columns from the CSV
struct TradeRecord{string date;long long key;long long value;long long cumulative;};

// Converts a date string in format DD/MM/YYYY into total days since 01/01/2015.
// Return value:
// - positive number: days since 01/01/2015
// - -1: invalid/too-short input
long long dateToDays(const string& dateStr){
    if(dateStr.length()<10) return -1;                    // reject malformed date
    int d = stoi(dateStr.substr(0,2));                    // day portion
    int m = stoi(dateStr.substr(3,2));                    // month portion
    int y = stoi(dateStr.substr(6,4));                    // year portion
    
    // Precomputed days that occur before each month in a non-leap year.
    static const int daysBeforeMonth[] = {0,0,31,59,90,120,151,181,212,243,273,304,334};
    
    // Start with the number of full years since 2015 times 365
    long long totalDays = (y - 2015) * 365;

    // Add leap days for each intervening year between 2015 and y (exclusive).
    for(int yr = 2015; yr < y; ++yr){
        // Leap year rule: divisible by 4 but not 100, unless divisible by 400
        if((yr % 4 == 0 && yr % 100 != 0) || (yr % 400 == 0)) totalDays++;
    }

    // Add days from the months within the target year, plus days in the month
    totalDays += daysBeforeMonth[m] + (d - 1);

    // If target year is a leap year and month > Feb, add one more day
    if(m > 2 && ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0))) totalDays++;

    return totalDays; // numeric key representing the date
}

// Parses a single CSV line into its comma-separated cells, handling quoted
// fields that may contain commas. This is a minimal robust CSV parser.
vector<string> parseCSVLine(const string& line){
    vector<string> result;       // parsed fields
    string cell;                 // accumulator for the current field
    bool inQuotes = false;       // whether we're inside a quoted field

    for(char c : line){
        if(c == '"'){
            // Toggle quote state when encountering a double quote
            inQuotes = !inQuotes;
        } else if(c == ',' && !inQuotes){
            // If comma outside quotes -> field separator
            result.push_back(cell);
            cell.clear();
        } else {
            // Regular character or comma inside quotes -> append
            cell += c;
        }
    }

    // Add last accumulated cell after loop completes
    result.push_back(cell);
    return result;
}

// 1. Binary Interpolation Search (BIS)
// This search combines interpolation's guess where a value might be with a
// stepping strategy to find the target quickly when keys are roughly
// uniformly distributed.
int binaryInterpolationSearch(const vector<TradeRecord>& arr, long long target, long long& steps){
    int low = 0;                             // leftmost index of current range
    int high = (int)arr.size() - 1;          // rightmost index of current range
    steps = 0;                               // counter for diagnostic steps

    // Loop while range is valid and target lies within endpoints' keys
    while(low <= high && target >= arr[low].key && target <= arr[high].key){
        steps++;                             // count this probe/iteration

        // If endpoints are equal, array slice is constant; check quickly
        if(arr[low].key == arr[high].key){
            if(arr[low].key == target) return low;
            break; // can't interpolate relative position if values equal
        }

        // Estimate fractional position of target relative to endpoints
        int size = high - low + 1;
        double ratio = (double)(target - arr[low].key) / (arr[high].key - arr[low].key);
        int pos = low + (int)(ratio * (size - 1)); // interpolated index guess

        // Safety: ensure pos is inside current window
        if(pos < low || pos > high) break;

        // Direct hit check
        if(arr[pos].key == target) return pos;

        // Use sqrt of range as a step size for local linear probing
        int stepSize = (int)sqrt(size);
        if(stepSize < 1) stepSize = 1;

        if(arr[pos].key < target){
            // Move forward in jumps until we pass or reach the target
            int i = pos;
            while(i <= high && arr[i].key < target){
                steps++;
                i += stepSize;
            }
            // Narrow the search window to between the last checked jump
            low = i - stepSize + 1;
            high = min(i, high);
        } else {
            // Move backward in jumps until we pass or reach the target
            int i = pos;
            while(i >= low && arr[i].key > target){
                steps++;
                i -= stepSize;
            }
            // Narrow search window accordingly
            high = i + stepSize - 1;
            low = max(i, low);
        }
    }

    // Not found -> return -1
    return -1;
}

// 2. Binary Interpolation Search Star (BIS*)
// BIS* is like BIS but bounds the number of sequential step jumps to avoid
// pathological O(sqrt(N)) behavior on adversarial distributions.
int binaryInterpolationSearchStar(const vector<TradeRecord>& arr, long long target, long long& steps){
    int low = 0;
    int high = (int)arr.size() - 1;
    steps = 0;
    const int MAX_JUMPS = 3; // limit on consecutive jumps before fallback

    while(low <= high && target >= arr[low].key && target <= arr[high].key){
        steps++;

        if(arr[low].key == arr[high].key){
            if(arr[low].key == target) return low;
            break;
        }

        int size = high - low + 1;
        double ratio = (double)(target - arr[low].key) / (arr[high].key - arr[low].key);
        int pos = low + (int)(ratio * (size - 1)); // interpolation guess

        if(pos < low || pos > high) break;
        if(arr[pos].key == target) return pos;

        int stepSize = (int)sqrt(size);
        if(stepSize < 1) stepSize = 1;

        int jumps = 0; // number of sequential jumps performed this iteration
        if(arr[pos].key < target){
            int i = pos;
            // Jump forward while limiting the number of jumps
            while(i <= high && arr[i].key < target && jumps < MAX_JUMPS){
                steps++;
                i += stepSize;
                jumps++;
            }

            if(jumps == MAX_JUMPS && i <= high && arr[i].key < target){
                // If we've reached max jumps and still not past target, do a
                // binary-like mid pivot to avoid too slow linear progression
                int mid = pos + (high - pos) / 2;
                if(arr[mid].key < target) low = mid + 1;
                else high = mid;
            } else {
                // Otherwise, narrow window to the region covered by jumps
                low = i - stepSize + 1;
                high = min(i, high);
            }
        } else {
            int i = pos;
            // Jump backward while limiting jumps
            while(i >= low && arr[i].key > target && jumps < MAX_JUMPS){
                steps++;
                i -= stepSize;
                jumps++;
            }

            if(jumps == MAX_JUMPS && i >= low && arr[i].key > target){
                // Fallback to a mid-based contraction if too many jumps occurred
                int mid = low + (pos - low) / 2;
                if(arr[mid].key > target) high = mid - 1;
                else low = mid;
            } else {
                // Narrow the window based on the performed backward jumps
                high = i + stepSize - 1;
                low = max(i, low);
            }
        }
    }

    return -1; // not found
}

int main(){
    // CSV filename expected to be in same folder as executable
    string filename = "effects-of-covid-19-on-trade-at-15-december-2021-provisional.csv";
    ifstream file(filename); // open file for reading

    // If opening fails, report error and exit
    if(!file.is_open()){
        cerr<<"Error opening file."<<endl;
        return 1;
    }

    vector<TradeRecord> dataset; // store parsed rows
    string line;                 // buffer for reading lines

    getline(file, line); // discard header row (assumed present)

    // Read data rows one by one
    while(getline(file, line)){
        if(line.empty()) continue;                  // skip blank lines
        vector<string> row = parseCSVLine(line);    // split CSV row into fields
        if(row.size() >= 10){                       // ensure enough columns
            TradeRecord rec;                        // new record to populate
            rec.date = row[2];                      // date is the 3rd column
            rec.key = dateToDays(rec.date);         // numeric key derived from date
            try{ rec.value = stoll(row[8]); } catch(...){ rec.value = 0; }         // parse value
            try{ rec.cumulative = stoll(row[9]); } catch(...){ rec.cumulative = 0; } // parse cumulative
            dataset.push_back(rec);                 // append to dataset
        }
    }
    file.close(); // close file when done reading

    // Sort dataset by the numeric `key` (days since base date)
    sort(dataset.begin(), dataset.end(), [](const TradeRecord &a, const TradeRecord &b){ return a.key < b.key; });

    // Example query date and its numeric conversion
    string inputDate = "15/06/2018";
    long long targetKey = dateToDays(inputDate);
    long long stepsBIS = 0, stepsBISStar = 0; // step counters for diagnostics

    // Time BIS
    auto t1 = chrono::high_resolution_clock::now();
    int idxBIS = binaryInterpolationSearch(dataset, targetKey, stepsBIS);
    auto t2 = chrono::high_resolution_clock::now();

    // Time BIS*
    int idxBISStar = binaryInterpolationSearchStar(dataset, targetKey, stepsBISStar);
    auto t3 = chrono::high_resolution_clock::now();

    // Print results: index found (or -1), steps taken, and time in nanoseconds
    cout<<"--- TASK 4 RESULTS ("<<inputDate<<") ---"<<endl;
    cout<<"BIS:  Index = "<<idxBIS<<" | Steps = "<<stepsBIS<<" | Time = "<<chrono::duration_cast<chrono::nanoseconds>(t2-t1).count()<<" ns"<<endl;
    cout<<"BIS*: Index = "<<idxBISStar<<" | Steps = "<<stepsBISStar<<" | Time = "<<chrono::duration_cast<chrono::nanoseconds>(t3-t2).count()<<" ns"<<endl;

    return 0;
}