//*****************************************************************************************//
//**                                                                                     **//
//**                              TIFLoader                                              **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_TIFLoader_Header
#define Class_TIFLoader_Header

struct IFDentry {
	//データの識別コード
	unsigned short tag = 0;
	//データの型※下記参照, コード1～コード12
	unsigned short dataType = 0;
	//データフィールドに含まれる値の数(配列数)
	unsigned int countField = 0;
	//各種データまたはバイト単位のオフセット値
	//4byte以内なら一つの数値そのもの, 以上なら配列の先頭アドレス
	unsigned int dataFieldOrPointer = 0;
};

struct IFD {
	unsigned short entryCount = 0;//IFDエントリ数
	IFDentry* entry = nullptr;    //entryCount個生成
	unsigned int IFDpointer = 0;//次のIFDへのポインタ, 無い場合0

	~IFD() {
		if (entry) {
			delete[] entry;
			entry = nullptr;
		}
	}
};

class TIFLoader {

private:
	unsigned int unResizeImageSizeX = 0;
	unsigned int unResizeImageSizeY = 0;
	IFD ifd;

	bool bigEndian = false;//true:順方向
	unsigned int convertUCHARtoUINT(unsigned char* byte);
	unsigned short convertUCHARtoSHORT(unsigned char* byte);
	unsigned long byteShift(int Field, int shift);
	void createImageFileDirectory(unsigned char* byte);

	const unsigned int imageNumChannel = 4;

	void resize(unsigned char* dstImage, unsigned char* srcImage,
		unsigned int dstWid, unsigned int dstHei,
		unsigned int srcWid, unsigned int srcNumChannel, unsigned int srcHei);

public:
	unsigned char* loadTIF(char* pass, unsigned int outWid, unsigned int outHei,
		char* errorMessage = nullptr);

	unsigned char* loadTIFInByteArray(unsigned char* byteArray, unsigned int byteSize,
		unsigned int outWid, unsigned int outHei, char* errorMessage = nullptr);

	unsigned int getSrcWidth() { return unResizeImageSizeX; }
	unsigned int getSrcHeight() { return unResizeImageSizeY; }
};

#endif
/*
コード１…BYTE型(１バイト整数)
コード２…ASCII型(１バイトのASCII文字)
コード３…SHORT型(２バイト短整数)
コード４…LONG型(４バイト長整数)
コード５…RATIONAL型(８バイト分数、４バイトの分子とそれに続く４バイトの分母)
コード６…SBYTE型(１バイト符号付き整数)
コード７…UNDEFINED型(あらゆる１バイトデータ)
コード８…SSHORT型(２バイト符号付き短整数)
コード９…SLONG型(４バイト符号付き長整数)
コード10…SRATIONAL型(８バイト符号付き分数、４バイトの分子とそれに続く４バイトの分母)
コード11…FLOAT型(４バイト実数、IEEE浮動小数点形式)
コード12…DOUBLE型(８バイト倍精度実数、IEEE倍精度浮動小数点形式)
*/