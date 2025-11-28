#ifndef HASHMAP_H
#define HASHMAP_H

#include <string>

//  hash function for strings
inline unsigned long hashString(const std::string& key) {
    unsigned long hash = 5381;
    for (char c : key) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    return hash;
}

//  hash function for integers
inline unsigned long hashInt(int key) {
    return static_cast<unsigned long>(key);
}

template <typename K, typename V>
class HashMap {
private:
    struct Entry {
        K key;
        V value;
        bool occupied;
        
        Entry() : key(K()), value(V()), occupied(false) {}
    };
    
    Entry* table;
    int capacity;
    int size;
    
    // hash function
    unsigned long hash(const K& key) const {
        if constexpr (std::is_same<K, std::string>::value) {
            return hashString(key);
        } else {
            return hashInt(static_cast<int>(key));
        }
    }
    
    // Get index from hash
    int getIndex(const K& key) const {
        return hash(key) % capacity;
    }
    
    // Find slot for key (for insertion/lookup)
    int findSlot(const K& key) const {
        int index = getIndex(key);
        int startIndex = index;
        
        // Linear probing
        do {
            if (!table[index].occupied) {
                return -1; // Key not found
            }
            if (table[index].key == key) {
                return index; // Key found
            }
            index = (index + 1) % capacity;
        } while (index != startIndex);
        
        return -1; // Table full, key not found
    }
    
    // Find empty slot for insertion
    int findEmptySlot(const K& key) const {
        int index = getIndex(key);
        int startIndex = index;
        
        // Linear probing for empty slot
        do {
            if (!table[index].occupied) {
                return index;
            }
            index = (index + 1) % capacity;
        } while (index != startIndex);
        
        return -1; // No empty slot found
    }
    
    // Resize and rehash when needed
    void resizeIfNeeded() {
        if (size * 2 >= capacity) { // Load factor 0.5
            int oldCapacity = capacity;
            Entry* oldTable = table;
            
            capacity *= 2;
            table = new Entry[capacity];
            size = 0;
            
            // Rehash all entries
            for (int i = 0; i < oldCapacity; i++) {
                if (oldTable[i].occupied) {
                    insert(oldTable[i].key, oldTable[i].value);
                }
            }
            
            delete[] oldTable;
        }
    }
    
public:
    // Constructor
    HashMap(int initialCapacity = 16) : capacity(initialCapacity), size(0) {
        table = new Entry[capacity];
    }
    
    // Destructor
    ~HashMap() {
        delete[] table;
    }
    
    // Copy Constructor
    HashMap(const HashMap& other) : capacity(other.capacity), size(other.size) {
        table = new Entry[capacity];
        for (int i = 0; i < capacity; i++) {
            table[i] = other.table[i];
        }
    }
    
    // Assignment Operator
    HashMap& operator=(const HashMap& other) {
        if (this != &other) {
            delete[] table;
            capacity = other.capacity;
            size = other.size;
            table = new Entry[capacity];
            for (int i = 0; i < capacity; i++) {
                table[i] = other.table[i];
            }
        }
        return *this;
    }
    
    // Insert or update key-value pair
    void insert(const K& key, const V& value) {
        resizeIfNeeded();
        
        int index = findSlot(key);
        if (index >= 0) {
            // Key exists, update value
            table[index].value = value;
        } else {
            // Key doesn't exist, insert new
            index = findEmptySlot(key);
            if (index >= 0) {
                table[index].key = key;
                table[index].value = value;
                table[index].occupied = true;
                size++;
            }
            // If index == -1, resize should have prevented this
        }
    }
    
    // Get value by key with default if not found
    V get(const K& key, const V& defaultValue = V()) const {
        int index = findSlot(key);
        if (index >= 0) {
            return table[index].value;
        }
        return defaultValue;
    }
    
    // Check if key exists
    bool contains(const K& key) const {
        return findSlot(key) >= 0;
    }
    
    // Remove key-value pair
    bool remove(const K& key) {
        int index = findSlot(key);
        if (index >= 0) {
            table[index].occupied = false;
            size--;
            return true;
        }
        return false;
    }
    
    // Get size
    int getSize() const {
        return size;
    }
    
    // Check if empty
    bool isEmpty() const {
        return size == 0;
    }
    
    // Clear all elements
    void clear() {
        for (int i = 0; i < capacity; i++) {
            table[i].occupied = false;
        }
        size = 0;
    }
};

#endif // HASHMAP_H
