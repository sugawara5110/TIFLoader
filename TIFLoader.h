//*****************************************************************************************//
//**                                                                                     **//
//**                              TIFLoader                                              **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_TIFLoader_Header
#define Class_TIFLoader_Header

struct IFDentry {
	//�f�[�^�̎��ʃR�[�h
	unsigned short tag = 0;
	//�f�[�^�̌^�����L�Q��, �R�[�h1�`�R�[�h12
	unsigned short dataType = 0;
	//�f�[�^�t�B�[���h�Ɋ܂܂��l�̐�(�z��)
	unsigned int countField = 0;
	//�e��f�[�^�܂��̓o�C�g�P�ʂ̃I�t�Z�b�g�l
	//4byte�ȓ��Ȃ��̐��l���̂���, �ȏ�Ȃ�z��̐擪�A�h���X
	unsigned int dataFieldOrPointer = 0;
};

struct IFD {
	unsigned short entryCount = 0;//IFD�G���g����
	IFDentry* entry = nullptr;    //entryCount����
	unsigned int IFDpointer = 0;//����IFD�ւ̃|�C���^, �����ꍇ0

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

	bool bigEndian = false;//true:������
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
�R�[�h�P�cBYTE�^(�P�o�C�g����)
�R�[�h�Q�cASCII�^(�P�o�C�g��ASCII����)
�R�[�h�R�cSHORT�^(�Q�o�C�g�Z����)
�R�[�h�S�cLONG�^(�S�o�C�g������)
�R�[�h�T�cRATIONAL�^(�W�o�C�g�����A�S�o�C�g�̕��q�Ƃ���ɑ����S�o�C�g�̕���)
�R�[�h�U�cSBYTE�^(�P�o�C�g�����t������)
�R�[�h�V�cUNDEFINED�^(������P�o�C�g�f�[�^)
�R�[�h�W�cSSHORT�^(�Q�o�C�g�����t���Z����)
�R�[�h�X�cSLONG�^(�S�o�C�g�����t��������)
�R�[�h10�cSRATIONAL�^(�W�o�C�g�����t�������A�S�o�C�g�̕��q�Ƃ���ɑ����S�o�C�g�̕���)
�R�[�h11�cFLOAT�^(�S�o�C�g�����AIEEE���������_�`��)
�R�[�h12�cDOUBLE�^(�W�o�C�g�{���x�����AIEEE�{���x���������_�`��)
*/