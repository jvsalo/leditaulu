template<class T, size_t S>
class CircularBuffer {
  public:
    CircularBuffer() : index(0), count(0) {}
    
    void push_item(T item) {
        index = (index + 1) % S;
        array[index] = item;
        if (count < S) count++;
    }

    T pop_item() {
        int old_index = index;
        if (count > 0) {
            index = index == 0 ? S - 1 : index - 1;
            count--;
        }
        return array[old_index];
    }

    T peek_item() {
        return array[index];
    }
  private:
    int index;
    int count;
    T array[S];
};
