# Data Structures 2026 – Trade Data Analysis

This repository contains a collection of C++ programs developed for the analysis of trade data using different sorting algorithms, searching algorithms, and data structures.

The programs process the following CSV dataset:

**Dataset:** `effects-of-covid-19-on-trade-at-15-december-2021-provisional.csv`
**Source:** [Stats NZ – Effects of COVID-19 on trade at 15 December 2021](https://www.stats.govt.nz/information-releases/effects-of-covid-19-on-trade-at-15-december-2021-provisional)

## 📁 Repository Structure

The project is divided into two main parts.

### Part 1 – Sorting and Searching Algorithms

Part 1 contains four standalone C++ programs that work with the same dataset and compare different algorithms.

#### Program 1 – Merge Sort vs Counting Sort

* Reads and parses the CSV dataset, including fields containing commas inside quoted strings.
* Sorts records according to the `cumulative` field.
* Implements **Merge Sort**.
* Implements **Counting Sort**.
* Includes a memory-safety check for Counting Sort and falls back to standard sorting when the value range is too large.
* Measures the execution time of both algorithms in milliseconds.

#### Program 2 – Heap Sort vs Quick Sort

* Extracts the `value` field as the sorting key.
* Implements an in-place **Max-Heap Sort**.
* Implements **Quick Sort** using Hoare-style partitioning with a middle-element pivot.
* Measures the execution time of both algorithms in milliseconds.

#### Program 3 – Binary Search vs Interpolation Search

* Converts dates in `DD/MM/YYYY` format into a numeric key based on the number of days since `01/01/2015`.
* Sorts the records according to this numeric date key.
* Implements **Binary Search**.
* Implements **Interpolation Search**.
* Searches for a target date.
* Outputs the found index, number of algorithmic steps, and execution time in nanoseconds.

#### Program 4 – Advanced Interpolation Variations

* Implements **Binary Interpolation Search (BIS)**.
* Implements **Binary Interpolation Search Star (BIS*)**.
* BIS* limits sequential jumps to a maximum of 3.
* Outputs the found index, number of steps, and execution time in nanoseconds.

---

### Part 2 – Advanced Data Structures

Part 2 is an interactive command-line application that loads the CSV records into memory and provides operations using different data structures.

#### AVL Tree – By Date

* Implements an AVL tree ordered by normalized date.
* Includes left and right rotations to maintain balance.
* Supports searching for a cumulative value.
* Supports modifying a cumulative value.
* Supports deleting a record by date.
* Can display the records in sorted tree order.

#### AVL Tree – By Cumulative

* Implements an AVL tree ordered by cumulative value.
* Allows multiple dates with the same cumulative value to be stored in the same tree node using vectors.
* Supports queries for the minimum and maximum cumulative values.

#### Hash Table

* Implements a hash table using **separate chaining** for collision handling.
* Uses a custom hash function based on the ASCII values of the date string.
* Supports searching, modifying, and deleting records.
* Can display the contents of the hash table buckets.

#### Interactive CLI

* Provides interactive menus for the different data structures.
* Continues running until the user chooses to exit.
* Handles invalid input to prevent the program from crashing.

## 🚀 How to Run

Make sure you have a C++ compiler installed, such as **GCC/g++**.

Place the CSV dataset in the same directory as the source files/executable:

```text
effects-of-covid-19-on-trade-at-15-december-2021-provisional.csv
```

Compile the required source file using `g++`.

For example, to compile Part 2:

```bash
g++ part2.cpp -o part2_app
```

Then run the program:

```bash
./part2_app
```

On Windows:

```bash
part2_app.exe
```

The same procedure can be used to compile and run the programs from Part 1 using their corresponding source files.
