
   
#pragma once

template<typename It>
class IteratorRange {
    It begin_;
    It end_;

public:
    IteratorRange(const It begin, const It end) : begin_(begin), end_(end) {};
    It begin() const noexcept{ return begin_; }
    It end() const noexcept{ return end_; }
};

template<typename It>
std::ostream& operator<<(std::ostream& os, const IteratorRange<It>& ir) {
    for (It it = ir.begin(); it != ir.end(); ++it) {
        os << *it;
    }
    return os;
}

template <typename It>
class Paginator {
    std::vector<IteratorRange<It>> _range;
    It begin_;
    It end_;

public:
    Paginator(const It begin, const It end, const size_t size) :begin_(begin), end_(end) {
        while (begin_ != end_) {
            int distance = std::distance(begin_, end_);

            if (distance > static_cast<int>(size)) {
                distance = size;
            }
            _range.push_back({ begin_, begin_ + distance });
            std::advance(begin_, distance);
        }
    };

    auto begin()const noexcept{
        return _range.begin();
    }
    auto end()const noexcept{
        return _range.end();
    }
};

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}
