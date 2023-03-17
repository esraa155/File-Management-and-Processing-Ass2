#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <exception>
#include <sstream>
#include <utility>
#include <algorithm>
#include <stack>
using namespace std;

int ctoi(char c[]) {
    return stoi(string(c));
}

class InvalidRecordNumber : public exception {
private:
    int recordNumber;

public:
    explicit InvalidRecordNumber(int _recordNumber) : recordNumber{_recordNumber} {}
};


class InvalidPairNumber : public exception {
private:
    int pairNumber;
public:
    explicit InvalidPairNumber(int _pairNumber) : pairNumber{_pairNumber} {}
};
class BTree {
private:
    // The b-tree's file path.
    string path;

    // The file that holds the b-tree nodes.
    fstream file;

    // The number of values one node can hold.
    int m;

    // The maximum number of records the b-tree file can hold.
    int numberOfRecords;

    // The number of characters allocated for each pair
    // in the b-tree file.
    int cellSize;
public:

    // Returns the number of characters a record in the b-tree file takes
    // based on the specified pair size and number of values that one record can hold.
    int recordSize() const { return cellSize + 2 * m * cellSize; }

    // Returns the number of characters a pair values takes in a record
    // based on the specified pair size.
    int pairSize() const
     { return 2 * cellSize; }
    // Initializes the b-tree file with the maximum number of
    // records it can hold, and the maximum number of values
    // one record can hold, and the size of each pair in the b-tree file.
    BTree(string _path, int _numberOfRecords, int _m, int _cellSize) :
    path{move(_path)},
        m{_m},
        numberOfRecords{_numberOfRecords},
        cellSize{_cellSize}
    {
        openFile();
        initialize();
    }

    // Closes the b-tree file.
    ~BTree()
    {
        file.close();
    }

    //  Inserts a new value in the b-tree.
    //  Returns the index of the record in the b-tree file.
    //  Returns -1 if insertion failed.
    //  Insertion fails if there are no enough empty
    //  records to complete the insertion.
    int insert(int recordId, int reference)
    {
        vector<pair<int, int>> current;

        // If the root is empty
        if (isEmpty(1))
        {
            // Insert in root

            // The next empty for the record at next empty
            int nextEmptyNext = cell(nextEmpty(), 1);

            // Update the next empty
            writeCell(nextEmptyNext, 0, 1);

            // Create the node
            current = node(1);

            // Insert the new pair
            current.emplace_back(recordId, reference);

            // Sort the node
            sort(current.begin(), current.end());

            // Write the node in root
            writeNode(current, 1);

            // Mark the root as leaf
            markLeaf(1, 0);

            // Return the index of the record in which the insertion happened
            // i.e. the root in this case
            return 1;
        }

        // Keep track of visited records to updateAfterInsert them after insertion
        stack<int> visited;

        // Search for recordId in every node in the b-tree
        // starting with the root
        int i = 1;
        bool found;
        while (!isLeaf(i))
        {
            visited.push(i);
            current = node(i);
            found = false;
            for (auto p: current)
            {
                // If a greater value is found
                if (p.first >= recordId)
                {
                    // B-Tree traversal
                    i = p.second;
                    found = true;
                    break;
                }
            }
            // B-Tree traversal
            if (!found) i = current.back().second;
        }

        current = node(i);

        // Insert the new pair
        current.emplace_back(recordId, reference);

        // Sort the node
        sort(current.begin(), current.end());

        int newFromSplitIndex = -1;

        // If record overflowed after insertion
        if (current.size() > m)
            newFromSplitIndex = split(i, current);
        else
        // Write the node in root
        writeNode(current, i);

        // If the insertion happened in root
        // Then there are no parents to updateAfterInsert
        if (i == 1) return i;

        // Otherwise, updateAfterInsert parents
        while (!visited.empty())
        {
            int lastVisitedIndex = visited.top();
            visited.pop();

            newFromSplitIndex = updateAfterInsert(lastVisitedIndex, newFromSplitIndex);
        }

        // Return the index of the inserted record
        // or -1 if insertion failed
        return i;
    }

    // Searches for the node with the given value.
    // Returns the index of the record in the b-tree.
    // Returns -1 if the given value is not found in any node.
    int search(int recordId)
    {
        if (isEmpty(1)) return -1;

        vector<pair<int, int>> current;

        // Search for recordId in every node in the b-tree
        // starting with the root
        int i = 1;
        bool found;
        while (!isLeaf(i))
        {
            current = node(i);
            found = false;
            for (auto p: current)
            {
                // If a greater value is found
                if (p.first >= recordId)
                {
                    // B-Tree traversal
                    i = p.second;
                    found = true;
                    break;
                }
            }

            // B-Tree traversal
            if (!found) i = current.back().second;
        }

        current = node(i);
        for (auto pair: current)
            if (pair.first == recordId)
                return pair.second;

        return -1;
    }

    // Prints the b-tree file in a table format.
    void display()
    {
         // For each record
        for (int i = 0; i < numberOfRecords; ++i)
        {
            // Read the record
            char record[recordSize()];
            file.seekg(i * recordSize(), ios::beg);
            file.read(record, recordSize());

            // Instead of printing the array of characters,
            // print each character one by one to avoid weird null-terminator issue
            for (char r: record) cout << r;

            cout << '\n';
        }
    }

    // Searches for and removes the given value from the b-tree.
    void remove(int recordId)
    {
        // If the root is empty
        if (isEmpty(1)) return;

        vector<pair<int, int>> current;

        // Keep track of visited records to updateAfterInsert them after insertion
        stack<int> visited;

        // Search for recordId in every node in the b-tree
        // starting with the root
        int currentRecordNumber = 1, parentRecordNumber = -1;
        bool found;
        while (!isLeaf(currentRecordNumber)) {
            visited.push(currentRecordNumber);
            current = node(currentRecordNumber);
            found = false;
            for (auto p: current) {
                // If a greater value is found
                if (p.first >= recordId) {
                    // B-Tree traversal
                    parentRecordNumber = currentRecordNumber;
                    currentRecordNumber = p.second;
                    found = true;
                    break;
                }
            }

            // B-Tree traversal
            if (!found) {
                parentRecordNumber = currentRecordNumber;
                currentRecordNumber = current.back().second;
            }
        }

        current = node(currentRecordNumber);

        // Delete first pair with first == recordId
        for (auto pair = current.begin(); pair != current.end(); ++pair)
            if (pair->first == recordId) {
                current.erase(pair);
                break;
            }


        if (current.size() < m / 2) {
            if (!redistribute(parentRecordNumber, currentRecordNumber, current)) {
                merge(parentRecordNumber, currentRecordNumber, current);
            }
        }else {
            writeNode(current, currentRecordNumber);
        }

    // Otherwise, updateAfterInsert parents
        while (!visited.empty()) {
            int lastVisitedIndex = visited.top();
            visited.pop();
            if (!visited.empty())
                updateAfterDelete(lastVisitedIndex, visited.top());
            else
                updateAfterDelete(lastVisitedIndex, -1);
        }
    }

    // Reads and returns the cell at the specified record and pair numbers.
    pair<int, int> _pair(int recordNumber, int pairNumber)
    {
        // Validate inputs
        validateRecordNumber(recordNumber);
        validatePairNumber(pairNumber);

        // Create and return the cell
        pair<int, int> thePair;
        thePair.first = cell(recordNumber, 2 * pairNumber - 1);
        thePair.second = cell(recordNumber, 2 * pairNumber);
        return thePair;
    }

    // Reads and returns the node at the specified record number.
    vector<pair<int, int>> node(int recordNumber)
    {
          // Validate input
        validateRecordNumber(recordNumber);

        // Go to the specified record on the b-tree file
        file.seekg(recordNumber * recordSize() + cellSize, ios::beg);

        // Create and return the node
        vector<pair<int, int>> theNode{};

        // Read every pair in the node
        for (int i = 1; i <= m; ++i) {
            pair<int, int> p = _pair(recordNumber, i);

            // If it is empty then the rest is empty, so return
            if (p.second == -1) return theNode;

            // Otherwise, continue reading the node
            theNode.push_back(p);
        }
        return theNode;
    }

    // Returns the value of the second pair in the header.
    // This value is the number of the first empty record
    // available for allocation.
    int nextEmpty()
    {
        return cell(0, 1);
    }

    // Returns true if the record's leaf status is equal to 0
    bool isLeaf(int recordNumber)
    {
        return cell(recordNumber, 0) == 0;
    }

    // Returns true if the record's leaf status is equal to -1
    bool isEmpty(int recordNumber)
    {
        return cell(recordNumber, 0) == -1;
    }
    // Splits the record into two.
    // Returns the number of the newly allocated record in the b-tree file.
    int split(int recordNumber, vector<pair<int, int >> originalNode)
    {
        if (recordNumber == 1)
            return split(originalNode);

        // Get the index of the new record created after split
        int newRecordNumber = nextEmpty();

        // If there are no empty records, then splitting fails
        if (newRecordNumber == -1) return -1;

        // Update the next empty cell with the next in available list
        writeCell(cell(newRecordNumber, 1), 0, 1);

        // Distribute originalNode on two new nodes
        vector<pair<int, int>> firstNode, secondNode;

        // Fill first and second nodes from originalNode
        auto middle(originalNode.begin() + (int) (originalNode.size()) / 2);
        for (auto it = originalNode.begin(); it != originalNode.end(); ++it)
        {
            if (distance(it, middle) > 0) firstNode.push_back(*it);
            else secondNode.push_back(*it);
        }

        // Clear originalNodeIndex and newNodeIndex
        clearRecord(recordNumber);
        clearRecord(newRecordNumber);

        markLeaf(recordNumber, 0);
        writeNode(firstNode, recordNumber);

        markLeaf(newRecordNumber, 0);
        writeNode(secondNode, newRecordNumber);

        return newRecordNumber;
    }

    // Splits the root into two and allocates a new root.
    bool split(vector<pair<int, int>> root)
    {
        // Find 2 empty records for the new nodes
        int firstNodeIndex = nextEmpty();
        if (firstNodeIndex == -1) return false;

        // Get next empty node in available list
        int secondNodeIndex = cell(firstNodeIndex, 1);
        if (secondNodeIndex == -1) return false;

        // Update the next empty cell with the next in available list
        writeCell(cell(secondNodeIndex, 1), 0, 1);

        vector<pair<int, int>> firstNode, secondNode;

        // Fill first and second nodes from root
        auto middle(root.begin() + (int) (root.size()) / 2);
        for (auto it = root.begin(); it != root.end(); ++it) {
            if (distance(it, middle) > 0) firstNode.push_back(*it);
            else secondNode.push_back(*it);
        }

        markLeaf(firstNodeIndex, leafStatus(1));
        writeNode(firstNode, firstNodeIndex);

        markLeaf(secondNodeIndex, leafStatus(1));
        writeNode(secondNode, secondNodeIndex);

        clearRecord(1);

        // Create new root with max values from the 2 new nodes
        vector<pair<int, int>> newRoot;
        newRoot.emplace_back(firstNode.back().first, firstNodeIndex);
        newRoot.emplace_back(secondNode.back().first, secondNodeIndex);
        markNonLeaf(1);
        writeNode(newRoot, 1);

        return true;
    }

    // Opens the b-tree's file.
    void openFile()
    {
         file.open("../btree",ios::trunc | ios::in | ios::out);
    }

    // Returns the integer value that the specified cell holds.
    int cell(int rowIndex, int columnIndex)
    {
        // Go to the specified cell
        file.seekg(rowIndex * recordSize() + columnIndex * cellSize, ios::beg);

        // Read and return the integer value in the cell
        char cell[cellSize];
        file.read(cell, cellSize);
        return ctoi(cell);
    }

    // Writes the given value in the specified cell
    // in the b-tree file.
    void writeCell(int value, int rowIndex, int columnIndex)
    {
        // Go to the specified cell
        file.seekg(rowIndex * recordSize() + columnIndex * cellSize, ios::beg);

        // Write the given value in the cell
        file << pad(value);
    }

    // Asserts the given record number is within a valid range.
    // This valid range depends on the maximum number of records
    // that the b-tree file can hold.
    void validateRecordNumber(int recordNumber) const
    {
        // If the record number is not between 1 and numberOfRecords
        if (recordNumber <= 0 || recordNumber > numberOfRecords)
        // then it is not a valid record number
        throw InvalidRecordNumber(recordNumber);
    }

    // Asserts the given pair number is within a valid range.
    // This valid range depends on the maximum number of values
    // a record in the b-tree file can hold.
    void validatePairNumber(int pairNumber) const
    {
        // If the pair number is not between 1 and m
        if (pairNumber <= 0 || pairNumber > m)
        // then it is not a valid pair number
        throw InvalidPairNumber(pairNumber);
    }

    // Initializes the b-tree file with -1s
    // and the available list
    void initialize()
    {
            // For each record
        for (int recordIndex = 0; recordIndex < numberOfRecords + 1; ++recordIndex) {
            // Write -1 in the first cell to indicate it is
            // an empty record (available for allocation)
            writeCell(-1, recordIndex, 0);

            // Write the number of the next empty record
            // in the available list
            if (recordIndex == numberOfRecords - 1)
                writeCell(-1, recordIndex, 1);
            else
                writeCell(recordIndex + 1, recordIndex, 1);

            // Fill the rest of the record with -1s
            for (int cellIndex = 2; cellIndex <= m * 2; ++cellIndex)
                writeCell(-1, recordIndex, cellIndex);
        }
    }

    // Returns the given value in a padded string
    // with the size of the cell in the b-tree file.
    string pad(int value) const
    {
        stringstream result;

        // Convert the integer value into a string
        string stringValue = to_string(value);

        // Write the string
        result << stringValue;

        // Write spaces until the final result's size becomes the cell size
        for (int i = 0; i < cellSize - stringValue.size(); ++i) result << ' ';
        return result.str();
    }

    void writeNode(const vector<pair<int, int>>& node, int recordNumber)
    {
        clearRecord(recordNumber);
        file.seekg(recordNumber * recordSize() + cellSize, ios::beg);
        for (auto p: node)
        file << pad(p.first) << pad(p.second);
    }

    void markLeaf(int recordNumber, int leafStatus)
    {
        file.seekg(recordNumber * recordSize(), ios::beg);
        file << pad(leafStatus);
    }

    int updateAfterInsert(int parentRecordNumber, int newChildRecordNumber)
    {
        vector<pair<int, int>> newParent;
        auto parent = node(parentRecordNumber);
        // For each value in parent
        for (auto p: parent) {
            auto childNode = node(p.second);
            // Add the maximum of the value's child
            newParent.emplace_back(childNode.back().first, p.second);
        }
    // If there was a new child from previous split
        if (newChildRecordNumber != -1)
        //  Add the maximum of the new value's child
        newParent.emplace_back(node(newChildRecordNumber).back().first, newChildRecordNumber);

        sort(newParent.begin(), newParent.end());

        int newFromSplitIndex = -1;

        // If record overflowed after insertion
        if (newParent.size() > m)
            newFromSplitIndex = split(parentRecordNumber, newParent);
        else
            // Write new parent
            writeNode(newParent, parentRecordNumber);

        return newFromSplitIndex;
    }

    void clearRecord(int recordNumber)
    {
        for (int i = 1; i <= m * 2; ++i)
        {
            file.seekg(recordNumber * recordSize() + i * cellSize, ios::beg);
            file << pad(-1);
        }
    }

    void markNonLeaf(int recordNumber)
    {
        file.seekg(recordNumber * recordSize(),ios::beg);
        file << pad(1);
    }

    int leafStatus(int recordNumber)
    {
        return cell(recordNumber, 0);
    }

    void markEmpty(int recordNumber)
    {
        file.seekg(recordNumber * recordSize(), ios::beg);
        file << pad(-1);
    }

    bool redistribute(int parentRecordNumber, int currentRecordNumber, vector<pair<int, int>> currentNode)
    {
        auto parent = node(parentRecordNumber);

        if (parent[0].second == currentRecordNumber)
        {
            return false;
        }

        // For each pair in parent node
        for (int i = 0; i < parent.size() - 1; ++i) {
        // If the pair after the current pair is pointing to the record where deletion happened
        // i.e. If we reached the pair to left of the pair where the deletion happened
        if (parent[i + 1].second == currentRecordNumber)
        {
            int siblingRecordNumber = parent[i].second;
            auto sibling = node(siblingRecordNumber);
            // Check the size of the child node of this pair
            // If it is going to be less than m/2 after redistribution, do nothing and return false
            if (sibling.size() == m / 2)
            {
                return false;
            }
            else
            {   // Otherwise, if we can redistribute
                // Take one pair from sibling and put it in the node where deletion happened
                    currentNode.push_back(sibling.back());

                    // Remove the moved pair from the sibling node
                    sibling.pop_back();

                    // Sort the current node and write both nodes
                    sort(currentNode.begin(), currentNode.end());
                    clearRecord(currentRecordNumber);
                    writeNode(currentNode, currentRecordNumber);
                    clearRecord(siblingRecordNumber);
                    writeNode(sibling, siblingRecordNumber);
                    return true;
                }
            }
        }
        return false;
    }

    void merge(int parentRecordNumber, int currentRecordNumber,vector<pair<int, int>> currentNode)
    {
        auto parent = node(parentRecordNumber);

        if (parent[0].second == currentRecordNumber)
        {
            if (parent.size() > 1)
            {
                int siblingRecordNumber = parent[1].second;
                auto sibling = node(siblingRecordNumber);
                while (!currentNode.empty())
                {
                    sibling.push_back(currentNode.back());
                    currentNode.pop_back();
                }
                sort(sibling.begin(), sibling.end());
                writeNode(sibling, siblingRecordNumber);
                clearRecord(currentRecordNumber);
                markEmpty(currentRecordNumber);
                int empty = nextEmpty();
                writeCell(currentRecordNumber, 0, 1);
                writeCell(empty, currentRecordNumber, 1);
            }
            return;
        }
        // For each pair in parent node
        for (int i = 0; i < parent.size() - 1; ++i)
        {
            // If the pair after the current pair is pointing to the record where deletion happened
            // i.e. If we reached the pair to left of the pair where the deletion happened
            if (parent[i + 1].second == currentRecordNumber)
            {
                int siblingRecordNumber = parent[i].second;
                auto sibling = node(siblingRecordNumber);
                while (!currentNode.empty())
                {
                    sibling.push_back(currentNode.back());
                    currentNode.pop_back();
                }
                sort(sibling.begin(), sibling.end());
                writeNode(sibling, siblingRecordNumber);
                clearRecord(currentRecordNumber);
                markEmpty(currentRecordNumber);
                int empty = nextEmpty();
                writeCell(currentRecordNumber, 0, 1);
                writeCell(empty, currentRecordNumber, 1);
                return;
            }
        }
    }

    void updateAfterDelete(int parentRecordNumber, int grandParentRecordNumber)
    {
        vector<pair<int, int>> newParent;

        auto parent = node(parentRecordNumber);

        // For each pair in parent
        for (auto p: parent)
        {
            auto childNode = node(p.second);
            if (!childNode.empty())
            // Add the maximum of the value's child
            newParent.emplace_back(childNode.back().first, p.second);
        }

        sort(newParent.begin(), newParent.end());

        // If record overflowed after insertion
        if (newParent.size() < m / 2 && grandParentRecordNumber != -1)
        {
//        newFromSplitIndex = split(parentRecordNumber, newParent);
            if (!redistribute(grandParentRecordNumber, parentRecordNumber, newParent))
            {
                merge(grandParentRecordNumber, parentRecordNumber, newParent);
            }
        }
        else
        // Write new parent
        writeNode(newParent, parentRecordNumber);
        }
};


int main() {
    BTree btree("../btree", 10, 5, 5);

    cout << "-----------------------------------------------------\n";

    btree.insert(3, 12);
    btree.insert(7, 24);
    btree.insert(10, 48);
    btree.insert(24, 60);
    btree.insert(14, 72);

    btree.display();

    cout << "-----------------------------------------------------\n";

    btree.insert(19, 84);

    btree.display();

    cout << "-----------------------------------------------------\n";

    btree.insert(30, 96);
    btree.insert(15, 108);
    btree.insert(1, 120);
    btree.insert(5, 132);

    btree.display();

    cout << "-----------------------------------------------------\n";

    btree.insert(2, 144);

    btree.display();

    cout << "-----------------------------------------------------\n";

    btree.insert(8, 156);
    btree.insert(9, 168);
    btree.insert(6, 180);
    btree.insert(11, 192);
    btree.insert(12, 204);
    btree.insert(17, 216);
    btree.insert(18, 228);

    btree.display();

    cout << "-----------------------------------------------------\n";

    btree.insert(32, 240);

    btree.display();

    cout << "-----------------------------------------------------\n";

    int target;
    target = 10;
    cout<<"Removed "<< target<<endl;
    btree.remove(target);
    btree.display();
    cout << "-----------------------------------------------------\n";

    target = 9;
    cout<<"Removed "<< target<<endl;
    btree.remove(target);
    btree.display();

    target = 8;
    cout<<"Removed "<< target<<endl;
    btree.remove(target);
    btree.display();



    return 0;
}




