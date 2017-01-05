#ifndef INTERFACE_MESSAGECODEC_HPP_
#define INTERFACE_MESSAGECODEC_HPP_

#include <vector>
#include <cstring>
#include <exception>
#include <iostream>
#include "MessageCoDecHelpers.hpp"

namespace ptree
{
namespace protocol_x
{

class SizeReader
{
public:
    SizeReader():mCurrentSize(0) {}

    void translate()
    {
    }

    template <typename T, typename... Tt>
    void translate(T& head, Tt&... tail)
    {
        sizeRead(head, mCurrentSize);
        translate(tail...);
    }

    uint32_t size()
    {
        return mCurrentSize;
    }
private:
    uint32_t mCurrentSize;
};

class Encoder
{
public:
    Encoder(BufferView& codedData):
        mEncodeCursor(codedData)
    {}

    void translate()
    {
    }

    template <typename T, typename... Tt>
    void translate(T& head, Tt&... tail)
    {
        encode(head, mEncodeCursor);
        translate(tail...);
    }

private:
    BufferView& mEncodeCursor;
};

class Decoder
{
public:
    Decoder(BufferView& bv):
        mDecodeCursor(bv)
    {}

    void translate()
    {
    }

    template <typename T, typename... Tt>
    void translate(T& head, Tt&... tail)
    {
        decode(head, mDecodeCursor);
        translate(tail...);
    }

private:
    BufferView& mDecodeCursor;
};

// THUG CPP
#define MESSAGE_FIELDS_PROTOX(...)\
template<typename T>\
void serialize(T& codec)\
{\
    codec.translate(__VA_ARGS__);\
}\
void generate(BufferView& data)\
{\
    Encoder en(data);\
    this->serialize(en);\
}\
void parse(BufferView& data)\
{\
    Decoder de(data);\
    this->serialize(de);\
}\
uint32_t size()\
{\
    SizeReader sr;\
    sr.translate(__VA_ARGS__);\
    return sr.size();\
}\
std::vector<uint8_t> getPacked()\
{\
    Buffer e(this->size());\
    BufferView v(e);\
    Encoder en(v);\
    this->serialize(en);\
    return e;\
}\
bool unpackFrom(std::vector<uint8_t>& message)\
{\
    BufferView bv(message);\
    Decoder de(bv);\
    this->serialize(de);\
    return true;\
}

// END THUG

} // namespace protocol
} // namespace ptree

#endif  // INTERFACE_MESSAGECODEC_HPP_