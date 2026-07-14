#include <vector>

template <typename T>
std::ostream& operator<<(std::ostream& stream, std::vector<T> vec) {
    stream << "{";

    if (vec.size() > 10) {
        for (size_t i = 0; i < 5; i++) {
            stream << vec[i] << ", ";
        }

        stream << "..., ";

        for (size_t i = vec.size() - 5; i < vec.size(); i++) {
            stream << vec[i];

            if (i < vec.size() - 1)
                stream << ", ";
        }
    }
    else {
        for (size_t i = 0; i < vec.size(); i++) {
            stream << vec[i];

            if (i < vec.size() - 1)
                stream << ", ";
        }
    }

    stream << "}";
    return stream;
}