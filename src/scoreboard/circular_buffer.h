template<class T, size_t S>
class CircularBuffer {
  public:
    CircularBuffer() : index(0), count(0) {}
    
    void push_item(T item) {
        array[index + 1] = item;
        index = (index + 1) % S;
        count++;
    }

    T pop_item() {
        if (count > 0) {
            int old_index = index;
            index = index == 0 ? S - 1 : index - 1;
            count--;
            return array[old_index];
        }
    }

    T peak_item() {
        if (count > 0) {
            return array[index];
        }
    }
  private:
    int index;
    int count;
    T array[S];
};
