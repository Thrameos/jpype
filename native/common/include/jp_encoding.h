#ifndef _JP_ENCODING_H_
#define _JP_ENCODING_H_

#include <iostream>
#include <string>
#include <sstream>

class JPEncoding
{
	public:
		JPEncoding() {}
		virtual ~JPEncoding()=0;

		/** Store a code point in an outgoing buffer. */
		virtual void encode(std::ostream& out, unsigned int codePoint)=0;

		/** Retrieve a coding point from an incoming buffer. */
		virtual unsigned int fetch(std::istream& is)=0;
};

class JPEncodingUTF8 : public JPEncoding
{
	public:
		/** Store a code point in an outgoing buffer. */
		void encode(std::ostream& out, unsigned int codePoint);

		/** Retrieve a coding point from an incoming buffer. */
		unsigned int fetch(std::istream& is);
};

class JPEncodingJavaUTF8 : public JPEncoding
{
	public:
		/** Store a code point in an outgoing buffer. */
		void encode(std::ostream& out, unsigned int codePoint);

		/** Retrieve a coding point from an incoming buffer. 
		 * returns the next coding point or -1 on invalid coding.
		 */
		unsigned int fetch(std::istream& is);
};

std::string transcribe(char* in, size_t len, 
		JPEncoding& sourceEncoding,
		JPEncoding& targetEncoding);

#endif // _JP_ENCODING_H_
