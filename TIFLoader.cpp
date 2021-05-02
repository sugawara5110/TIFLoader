//*****************************************************************************************//
//**                                                                                     **//
//**                              TIFLoader                                              **//
//**                                                                                     **//
//*****************************************************************************************//

#define _CRT_SECURE_NO_WARNINGS
#include "TIFLoader.h"
#include <stdio.h>
#include <memory>

typedef const unsigned short tag;
static tag ImageWidth = 256;
static tag ImageLength = 257;
static tag BitsPerSample = 258;
static tag Compression = 259;
static tag PhotometricInterpretation = 262;
static tag StripOffsets = 273;
static tag RowsPerStrip = 278;
static tag StripByteCounts = 279;
static tag XResolusion = 282;
static tag YResolusion = 283;
static tag ResolutionUnit = 296;
static tag ColorMap = 320;

typedef const unsigned short dtype;
static dtype Byte = 1;
static dtype ASCII = 2;
static dtype Short = 3;
static dtype Long = 4;
static dtype RATIONAL = 5;
static dtype SBYTE = 6;
static dtype UNDEFINED = 7;
static dtype SSHORT = 8;
static dtype SLONG = 9;
static dtype SRATIONAL = 10;
static dtype Float = 11;
static dtype Double = 12;

unsigned int TIFLoader::convertUCHARtoUINT(unsigned char* byte) {
	int ind[4] = { 0,1,2,3 };
	if (!bigEndian) {
		ind[0] = 3;
		ind[1] = 2;
		ind[2] = 1;
		ind[3] = 0;
	}
	return((unsigned int)byte[ind[0]] << 24) | ((unsigned int)byte[ind[1]] << 16) |
		((unsigned int)byte[ind[2]] << 8) | ((unsigned int)byte[ind[3]]);
}

unsigned short TIFLoader::convertUCHARtoSHORT(unsigned char* byte) {
	int ind[2] = { 0,1 };
	if (!bigEndian) {
		ind[0] = 1;
		ind[1] = 0;
	}
	return((unsigned short)byte[ind[0]] << 8) | ((unsigned short)byte[ind[1]]);
}

unsigned long TIFLoader::byteShift(int Field, int shift) {
	if (bigEndian)
		return Field >> shift;
	return Field;
}

static void setEr(char* errorMessage, char* inMessage) {
	if (errorMessage) {
		int size = (int)strlen(inMessage) + 1;
		memcpy(errorMessage, inMessage, size);
	}
}

unsigned char* TIFLoader::loadTIF(char* pass, unsigned int outWid, unsigned int outHei, char* errorMessage) {
	FILE* fp = fopen(pass, "rb");
	if (!fp) {
		setEr(errorMessage, "File read error");
		return nullptr;
	}

	unsigned int count = 0;
	while (fgetc(fp) != EOF) {
		count++;
	}
	fseek(fp, 0, SEEK_SET);//�ŏ��ɖ߂�
	std::unique_ptr<unsigned char[]> byte = std::make_unique<unsigned char[]>(count);
	fread(byte.get(), sizeof(unsigned char), count, fp);
	fclose(fp);//�ǂݏI�����̂ŕ���

	return loadTIFInByteArray(byte.get(), count, outWid, outHei, errorMessage);
}

unsigned char* TIFLoader::loadTIFInByteArray(unsigned char* byte, unsigned int byteSize,
	unsigned int outWid, unsigned int outHei, char* errorMessage) {

	const short dir4D4D = 0x4d4d;//4D4D:��ʂ��牺�ʂ̏�
	const short dir4949 = 0x4949;//4949:�t
	const short type002A = 0x002a;//TIFF
	const short type002B = 0x002b;//BigTIFF

	unsigned short dir = 0;
	unsigned short type = 0;

	dir = convertUCHARtoSHORT(&byte[0]);

	if (dir4D4D == dir)bigEndian = true;

	type = convertUCHARtoSHORT(&byte[2]);

	unsigned int IFDpointer = convertUCHARtoUINT(&byte[4]);

	createImageFileDirectory(&byte[IFDpointer]);

	unsigned char* data = nullptr;//�ꎞ�ϐ�

	short SamplesPerPixel = 0;//1�s�N�Z���̐F��
	std::unique_ptr<short[]> bitsPerSample = nullptr;//�e�F��bit��
	long CompressionCode = 0;//���k�^�C�v
	short photometricInterpretation = 0;
	//0:�����[�h���m�N��, �s�N�Z���l�F�����O�A����(�QBitsPerSample - 1)
	//1:�����[�h���m�N��, �s�N�Z���l�F����(�QBitsPerSample - 1)�A�����O
	//2:RGB�_�C���N�g�J���[, �ŏ��l���O�A�ő�l��(�QBitsPerSample - 1)
	//3:�J���[�}�b�v
	//4:�_���}�X�N

	long numStrip = 0;
	std::unique_ptr<unsigned long[]> bitMapStripPointer = nullptr;//bitMap�ւ̃A�h���X
	std::unique_ptr<unsigned long[]> stripByteCounts = nullptr;//�X�g���b�v���Ƃ̃o�C�g��
	unsigned long rowsPerStrip = 0;//�e�X�g���b�v�̍s�� rowsPerStrip �~ ImageWidth = �X�g���b�v�̍ő�T�C�Y(8~64KB)
	unsigned long xResolusion[2] = {};//[0]:���q(�s�N�Z����) / [1]:����(����P��), ���̒l��ImageWidth�{ = �摜��(�g��Ȃ�����)
	unsigned long yResolusion[2] = {};//[0]:���q(�s�N�Z����) / [1]:����(����P��), ���̒l��Imagelength�{ = �摜����(�g��Ȃ�����)
	unsigned long resolutionUnit = 0;
	//�R�[�h�P�c�P�ʂȂ�(�����v���O�����ɔC����)
	//�R�[�h�Q�c�C���`(inch)�P��
	//�R�[�h�R�c�Z���`���[�g��(cm)�P��

	long numcolorMap = 0;
	std::unique_ptr<unsigned short[]> colorMap = nullptr;//�J���[�}�b�v

	for (int i = 0; i < ifd.entryCount; i++) {
		IFDentry& e = ifd.entry[i];
		switch (e.tag) {
		case ImageWidth:
			if (e.dataType == Short)
				unResizeImageSizeX = (int)byteShift(e.dataFieldOrPointer, 16);//Short�̏ꍇ���2byte�̂ݗL��
			else
				unResizeImageSizeX = e.dataFieldOrPointer;//Long�̏ꍇ�S�ėL��
			break;
		case ImageLength:
			if (e.dataType == Short)
				unResizeImageSizeY = (int)byteShift(e.dataFieldOrPointer, 16);
			else
				unResizeImageSizeY = e.dataFieldOrPointer;
			break;
		case BitsPerSample:
			SamplesPerPixel = (short)e.countField;
			bitsPerSample = std::make_unique<short[]>(SamplesPerPixel);
			if (SamplesPerPixel == 1) {
				bitsPerSample[0] = (short)byteShift(e.dataFieldOrPointer, 16);
				break;
			}
			data = &byte[e.dataFieldOrPointer];
			for (short i1 = 0; i1 < SamplesPerPixel; i1++)bitsPerSample[i1] = convertUCHARtoSHORT(&data[i1 * 2]);
			break;
		case Compression:
			CompressionCode = byteShift(e.dataFieldOrPointer, 16);
			break;
		case PhotometricInterpretation:
			photometricInterpretation = (short)byteShift(e.dataFieldOrPointer, 16);
			break;
		case StripOffsets:
			//���̎��_�ł͊e�X�g���b�v�T�C�Y���擾�O�̉\��������̂�
			//�ebitmapStrip�̃|�C���^�z����쐬���邾���ɂ���
			numStrip = e.countField;
			bitMapStripPointer = std::make_unique<unsigned long[]>(numStrip);
			//numStrip == 1�̏ꍇbipmat�ւ̃|�C���^�������Ă�̂ł��̂܂܎g��
			if (numStrip == 1) {
				bitMapStripPointer[0] = (unsigned long)e.dataFieldOrPointer;//bitmap�ւ̃|�C���^
				break;
			}
			//numStrip == 1�ȊO�̏ꍇ, �I�t�Z�b�g�e�[�u���ւ̃|�C���^�������Ă�̂�
			//�e�I�t�Z�b�g����ebitmap�ւ̃|�C���^���擾
			for (long i1 = 0; i1 < numStrip; i1++) {
				if (e.dataType == Short)
					bitMapStripPointer[i1] = (unsigned long)convertUCHARtoSHORT(&byte[e.dataFieldOrPointer + i1 * 2]);//short
				else
					bitMapStripPointer[i1] = (unsigned long)convertUCHARtoUINT(&byte[e.dataFieldOrPointer + i1 * 4]);//long
			}
			break;
		case RowsPerStrip:
			if (e.dataType == Short)
				rowsPerStrip = byteShift(e.dataFieldOrPointer, 16);//short
			else
				rowsPerStrip = (unsigned long)e.dataFieldOrPointer;//long
			break;
		case StripByteCounts:
			numStrip = e.countField;
			stripByteCounts = std::make_unique<unsigned long[]>(numStrip);
			if (numStrip == 1) {
				stripByteCounts[0] = (unsigned long)e.dataFieldOrPointer;
				break;
			}
			for (long i1 = 0; i1 < numStrip; i1++) {
				if (e.dataType == Short)
					stripByteCounts[i1] = (unsigned long)convertUCHARtoSHORT(&byte[e.dataFieldOrPointer + i1 * 2]);//short
				else
					stripByteCounts[i1] = (unsigned long)convertUCHARtoUINT(&byte[e.dataFieldOrPointer + i1 * 4]);//long
			}
			break;
		case XResolusion:
			xResolusion[0] = (unsigned long)convertUCHARtoUINT(&byte[e.dataFieldOrPointer]);
			xResolusion[1] = (unsigned long)convertUCHARtoUINT(&byte[e.dataFieldOrPointer + 4]);
			break;
		case YResolusion:
			yResolusion[0] = (unsigned long)convertUCHARtoUINT(&byte[e.dataFieldOrPointer]);
			yResolusion[1] = (unsigned long)convertUCHARtoUINT(&byte[e.dataFieldOrPointer + 4]);
			break;
		case ResolutionUnit:
			resolutionUnit = byteShift(e.dataFieldOrPointer, 16);
			break;
		case ColorMap:
			numcolorMap = (long)pow(2, bitsPerSample[0]) * 3;
			colorMap = std::make_unique<unsigned short[]>(numcolorMap);
			for (long i1 = 0; i1 < numcolorMap; i1++) {
				colorMap[i1] = convertUCHARtoSHORT(&byte[e.dataFieldOrPointer + i1 * 2]);
			}
			break;
		}
	}

	unsigned int outbyteIndex = 0;
	unsigned char* outbyte3 = nullptr;
	switch (photometricInterpretation) {
	case 0://�����[�h���m�N��
		setEr(errorMessage, "�����[�h���m�N�����Ή��ł�");
		return nullptr;
		break;
	case 1://�����[�h���m�N��
		setEr(errorMessage, "�����[�h���m�N�����Ή��ł�");
		return nullptr;
		break;
	case 2://RGB�_�C���N�g�J���[
		outbyte3 = new unsigned char[unResizeImageSizeY * unResizeImageSizeX * 3];
		for (long i = 0; i < numStrip; i++) {
			memcpy(&outbyte3[outbyteIndex], &byte[bitMapStripPointer[i]], sizeof(unsigned char) * stripByteCounts[i]);
			outbyteIndex += stripByteCounts[i];
		}
		break;
	case 3://�J���[�}�b�v
		setEr(errorMessage, "�J���[�}�b�v���Ή��ł�");
		return nullptr;
		break;
	case 4://�_���}�X�N
		setEr(errorMessage, "�_���}�X�N���Ή��ł�");
		return nullptr;
		break;
	}
	if (outWid <= 0 || outHei <= 0) {
		outWid = unResizeImageSizeX;
		outHei = unResizeImageSizeY;
	}
	const unsigned int numPixel = outWid * imageNumChannel * outHei;
	unsigned char* image = new unsigned char[numPixel];//�O���ŊJ��
	resize(image, outbyte3, outWid, outHei, unResizeImageSizeX, 3, unResizeImageSizeY);

	delete[] outbyte3;
	outbyte3 = nullptr;

	setEr(errorMessage, "OK");

	return image;
}

void TIFLoader::createImageFileDirectory(unsigned char* byte) {
	ifd.entryCount = convertUCHARtoSHORT(&byte[0]);
	ifd.entry = new IFDentry[ifd.entryCount];
	for (int i = 0; i < ifd.entryCount; i++) {
		IFDentry& e = ifd.entry[i];
		unsigned char* entry = &byte[2] + sizeof(IFDentry) * i;
		e.tag = convertUCHARtoSHORT(entry);
		e.dataType = convertUCHARtoSHORT(&entry[2]);
		e.countField = convertUCHARtoUINT(&entry[4]);
		e.dataFieldOrPointer = convertUCHARtoUINT(&entry[8]);
	}
	ifd.IFDpointer = convertUCHARtoUINT(&byte[2] + sizeof(IFDentry) * ifd.entryCount);
}

void TIFLoader::resize(unsigned char* dstImage, unsigned char* srcImage,
	unsigned int dstWid, unsigned int dstHei,
	unsigned int srcWid, unsigned int srcNumChannel, unsigned int srcHei) {

	unsigned int dWid = dstWid * imageNumChannel;
	unsigned int sWid = srcWid * srcNumChannel;

	for (unsigned int dh = 0; dh < dstHei; dh++) {
		unsigned int sh = (unsigned int)((float)srcHei / (float)dstHei * dh);
		for (unsigned int dw = 0; dw < dstWid; dw++) {
			unsigned int dstInd0 = dWid * dh + dw * imageNumChannel;
			unsigned int sw = (unsigned int)((float)srcWid / (float)dstWid * dw) * srcNumChannel;
			unsigned int srcInd0 = sWid * sh + sw;
			dstImage[dstInd0 + 0] = srcImage[srcInd0];
			dstImage[dstInd0 + 1] = srcImage[srcInd0 + 1];
			dstImage[dstInd0 + 2] = srcImage[srcInd0 + 2];
			dstImage[dstInd0 + 3] = 255;
		}
	}
}