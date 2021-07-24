/*  
	Copyright (C) 2006-2007 shash

    This file is part of DeSmuME

    DeSmuME is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    DeSmuME is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with DeSmuME; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "matrix.h"

#define _isAligned(x) (((u32)x & 0xF)==0)

void MatrixInit  (float *matrix)
{
	memset (matrix, 0, sizeof(float)*16);
	matrix[0] = matrix[5] = matrix[10] = matrix[15] = 1.f;
/*
	__asm__ volatile (
		"vmidt.q M000"					"\n\t"
		"usv.q C000, %0"				"\n\t"
		"usv.q C010, 16 + %0"		"\n\t"
		"usv.q C020, 32 + %0"		"\n\t"
		"usv.q C030, 48 + %0"		"\n\t"
	: "=m" (*matrix)
	);*/
}

#ifdef PSP
void MATRIXFASTCALL MatrixMultVec4x4 (const float *matrix, float *vecPtr)
{/*
	float x = vecPtr[0];
	float y = vecPtr[1];
	float z = vecPtr[2];
	float w = vecPtr[3];

	vecPtr[0] = x * matrix[0] + y * matrix[4] + z * matrix[ 8] + w * matrix[12];
	vecPtr[1] = x * matrix[1] + y * matrix[5] + z * matrix[ 9] + w * matrix[13];
	vecPtr[2] = x * matrix[2] + y * matrix[6] + z * matrix[10] + w * matrix[14];
	vecPtr[3] = x * matrix[3] + y * matrix[7] + z * matrix[11] + w * matrix[15];
*/

	__asm__ volatile (
		"ulv.q C000, %1"						"\n\t"
		"ulv.q C010, 16 + %1"				"\n\t"
		"ulv.q C020, 32 + %1"				"\n\t"
		"ulv.q C030, 48 + %1"				"\n\t"

		"ulv.q R100, %0"						"\n\t"

		"vdot.q S200, R000, R100"		"\n\t"
		"vdot.q S201, R001, R100"		"\n\t"
		"vdot.q S202, R002, R100"		"\n\t"
		"vdot.q S203, R003, R100"		"\n\t"

		"usv.q C200, %0"						"\n\t"
	: "+m" (*vecPtr) : "m"(*matrix)
	);

}

void MATRIXFASTCALL MatrixMultVec3x3 (const float *matrix, float *vecPtr)
{/*
	float x = vecPtr[0];
	float y = vecPtr[1];
	float z = vecPtr[2];

	vecPtr[0] = x * matrix[0] + y * matrix[4] + z * matrix[ 8];
	vecPtr[1] = x * matrix[1] + y * matrix[5] + z * matrix[ 9];
	vecPtr[2] = x * matrix[2] + y * matrix[6] + z * matrix[10];*/

	__asm__ volatile (
		"lv.s S000, %1"						"\n\t"
		"lv.s S001, 4 + %1"				"\n\t"
		"lv.s S002, 8 + %1"				"\n\t"

		"lv.s S010, 12 + %1"			"\n\t"
		"lv.s S011, 16 + %1"			"\n\t"
		"lv.s S012, 20 + %1"			"\n\t"

		"lv.s S020, 24 + %1"			"\n\t"
		"lv.s S021, 28 + %1"			"\n\t"
		"lv.s S022, 32 + %1"			"\n\t"


		"lv.s S100, %0"						"\n\t"
		"lv.s S101, 4 + %0"				"\n\t"
		"lv.s S102, 8 + %0"				"\n\t"


		"vdot.t S200, R000, R100"	"\n\t"
		"vdot.t S201, R001, R100"	"\n\t"
		"vdot.t S202, R002, R100"	"\n\t"

		"sv.s S200, %0"						"\n\t"
		"sv.s S201, 4 + %0"				"\n\t"
		"sv.s S202, 8 + %0"				"\n\t"
	: "+m" (*vecPtr) : "m"(*matrix)
	);

}

void MATRIXFASTCALL MatrixMultiply (float *matrix, const float *rightMatrix)
{/*
	float tmpMatrix[16];

	tmpMatrix[0]  = (matrix[0]*rightMatrix[0])+(matrix[4]*rightMatrix[1])+(matrix[8]*rightMatrix[2])+(matrix[12]*rightMatrix[3]);
	tmpMatrix[1]  = (matrix[1]*rightMatrix[0])+(matrix[5]*rightMatrix[1])+(matrix[9]*rightMatrix[2])+(matrix[13]*rightMatrix[3]);
	tmpMatrix[2]  = (matrix[2]*rightMatrix[0])+(matrix[6]*rightMatrix[1])+(matrix[10]*rightMatrix[2])+(matrix[14]*rightMatrix[3]);
	tmpMatrix[3]  = (matrix[3]*rightMatrix[0])+(matrix[7]*rightMatrix[1])+(matrix[11]*rightMatrix[2])+(matrix[15]*rightMatrix[3]);

	tmpMatrix[4]  = (matrix[0]*rightMatrix[4])+(matrix[4]*rightMatrix[5])+(matrix[8]*rightMatrix[6])+(matrix[12]*rightMatrix[7]);
	tmpMatrix[5]  = (matrix[1]*rightMatrix[4])+(matrix[5]*rightMatrix[5])+(matrix[9]*rightMatrix[6])+(matrix[13]*rightMatrix[7]);
	tmpMatrix[6]  = (matrix[2]*rightMatrix[4])+(matrix[6]*rightMatrix[5])+(matrix[10]*rightMatrix[6])+(matrix[14]*rightMatrix[7]);
	tmpMatrix[7]  = (matrix[3]*rightMatrix[4])+(matrix[7]*rightMatrix[5])+(matrix[11]*rightMatrix[6])+(matrix[15]*rightMatrix[7]);

	tmpMatrix[8]  = (matrix[0]*rightMatrix[8])+(matrix[4]*rightMatrix[9])+(matrix[8]*rightMatrix[10])+(matrix[12]*rightMatrix[11]);
	tmpMatrix[9]  = (matrix[1]*rightMatrix[8])+(matrix[5]*rightMatrix[9])+(matrix[9]*rightMatrix[10])+(matrix[13]*rightMatrix[11]);
	tmpMatrix[10] = (matrix[2]*rightMatrix[8])+(matrix[6]*rightMatrix[9])+(matrix[10]*rightMatrix[10])+(matrix[14]*rightMatrix[11]);
	tmpMatrix[11] = (matrix[3]*rightMatrix[8])+(matrix[7]*rightMatrix[9])+(matrix[11]*rightMatrix[10])+(matrix[15]*rightMatrix[11]);

	tmpMatrix[12] = (matrix[0]*rightMatrix[12])+(matrix[4]*rightMatrix[13])+(matrix[8]*rightMatrix[14])+(matrix[12]*rightMatrix[15]);
	tmpMatrix[13] = (matrix[1]*rightMatrix[12])+(matrix[5]*rightMatrix[13])+(matrix[9]*rightMatrix[14])+(matrix[13]*rightMatrix[15]);
	tmpMatrix[14] = (matrix[2]*rightMatrix[12])+(matrix[6]*rightMatrix[13])+(matrix[10]*rightMatrix[14])+(matrix[14]*rightMatrix[15]);
	tmpMatrix[15] = (matrix[3]*rightMatrix[12])+(matrix[7]*rightMatrix[13])+(matrix[11]*rightMatrix[14])+(matrix[15]*rightMatrix[15]);

	memcpy (matrix, tmpMatrix, sizeof(float)*16);
*/

// i just learned vfpu today so please bear with me
// if you see that my long computation can be done in 1 line

	__asm__ volatile (
		"ulv.q C000, %0"					"\n\t"
		"ulv.q C010, 16 + %0"			"\n\t"
		"ulv.q C020, 32 + %0"			"\n\t"
		"ulv.q C030, 48 + %0"			"\n\t"

		"ulv.q R100, %1"					"\n\t"
		"ulv.q R101, 16 + %1"			"\n\t"
		"ulv.q R102, 32 + %1"			"\n\t"
		"ulv.q R103, 48 + %1"			"\n\t"

		"vdot.q S200, R000, R100"	"\n\t"
		"vdot.q S201, R001, R100"	"\n\t"
		"vdot.q S202, R002, R100"	"\n\t"
		"vdot.q S203, R003, R100"	"\n\t"

		"vdot.q S210, R000, R101"	"\n\t"
		"vdot.q S211, R001, R101"	"\n\t"
		"vdot.q S212, R002, R101"	"\n\t"
		"vdot.q S213, R003, R101"	"\n\t"

		"vdot.q S220, R000, R102"	"\n\t"
		"vdot.q S221, R001, R102"	"\n\t"
		"vdot.q S222, R002, R102"	"\n\t"
		"vdot.q S223, R003, R102"	"\n\t"

		"vdot.q S230, R000, R103"	"\n\t"
		"vdot.q S231, R001, R103"	"\n\t"
		"vdot.q S232, R002, R103"	"\n\t"
		"vdot.q S233, R003, R103"	"\n\t"

		"usv.q C200, %0"					"\n\t"
		"usv.q C210, 16 + %0"			"\n\t"
		"usv.q C220, 32 + %0"			"\n\t"
		"usv.q C230, 48 + %0"			"\n\t"
	: "+m" (*matrix) : "m"(*rightMatrix)
	);

}

void MATRIXFASTCALL MatrixTranslate	(float *matrix, const float *ptr)
{/*
	matrix[12] += (matrix[0]*ptr[0])+(matrix[4]*ptr[1])+(matrix[ 8]*ptr[2]);
	matrix[13] += (matrix[1]*ptr[0])+(matrix[5]*ptr[1])+(matrix[ 9]*ptr[2]);
	matrix[14] += (matrix[2]*ptr[0])+(matrix[6]*ptr[1])+(matrix[10]*ptr[2]);
	matrix[15] += (matrix[3]*ptr[0])+(matrix[7]*ptr[1])+(matrix[11]*ptr[2]);
*/
	__asm__ volatile (
		"ulv.q C000, %0"					"\n\t"
		"ulv.q C010, 16 + %0"			"\n\t"
		"ulv.q C020, 32 + %0"			"\n\t"
		"ulv.q C030, 48 + %0"			"\n\t"

		"lv.s S100, %1"						"\n\t"
		"lv.s S110, 4 + %1"				"\n\t"
		"lv.s S120, 8 + %1"				"\n\t"

		"vdot.t S110, R000, R100"	"\n\t"
		"vdot.t S111, R001, R100"	"\n\t"
		"vdot.t S112, R002, R100"	"\n\t"
		"vdot.t S113, R003, R100"	"\n\t"

		"vadd.q C030, C030, C110"	"\n\t"

		"usv.q C030, 48 + %0"			"\n\t"
	: "+m" (*matrix) : "m"(*ptr)
	);
}

void MATRIXFASTCALL MatrixScale (float *matrix, const float *ptr)
{/*
	matrix[0]  *= ptr[0];
	matrix[1]  *= ptr[0];
	matrix[2]  *= ptr[0];
	matrix[3]  *= ptr[0];

	matrix[4]  *= ptr[1];
	matrix[5]  *= ptr[1];
	matrix[6]  *= ptr[1];
	matrix[7]  *= ptr[1];

	matrix[8] *= ptr[2];
	matrix[9] *= ptr[2];
	matrix[10] *= ptr[2];
	matrix[11] *= ptr[2];*/

	__asm__ volatile (
		"lv.s S000, %1"						"\n\t"
		"lv.s S001, 4 + %1"				"\n\t"
		"lv.s S002, 8 + %1"				"\n\t"

		"ulv.q C010, %1"					"\n\t"
		"ulv.q C020, 16 + %1"			"\n\t"
		"ulv.q C030, 32 + %1"			"\n\t"

		"vscl.q C010, C010, S000"	"\n\t"
		"vscl.q C020, C020, S001"	"\n\t"
		"vscl.q C030, C030, S002"	"\n\t"

		"usv.q C010, %1"					"\n\t"
		"usv.q C020, 16 + %1"			"\n\t"
		"usv.q C030, 32 + %1"			"\n\t"
	: "+m" (*matrix) : "m"(*ptr)
	);

}
#endif //switched c/asm functions
//-----------------------------------------

void MatrixTranspose(float *matrix)
{/*
	float temp;
#define swap(A,B) temp = matrix[A];matrix[A] = matrix[B]; matrix[B] = temp;
	swap(1,4);
	swap(2,8);
	swap(3,0xC);
	swap(6,9);
	swap(7,0xD);
	swap(0xB,0xE);
#undef swap


	0 1 2 3		0 4 8 c
	4 5 6 7		1 5 9 7
	8 9 a b		2 6 a e
	c d e f		3 d b f
*/
	__asm__ volatile (
		"ulv.q C000, %0"				"\n\t"
		"ulv.q C010, 16 + %0"		"\n\t"
		"ulv.q C020, 32 + %0"		"\n\t"
		"ulv.q C030, 48 + %0"		"\n\t"
		"vmmov.q M100, M000"		"\n\t"
		"vmov.q R100, C000"			"\n\t"
		"vmov.q C100, R000"			"\n\t"
		"vmov.s S113, S031"			"\n\t"
		"vmov.s S131, S013"			"\n\t"
		"vmov.s S123, S032"			"\n\t"
		"vmov.s S132, S023"			"\n\t"
	:: "m" (*matrix)
	);
	
	if (_isAligned(matrix)){
		__asm__ volatile (
			"sv.q C100, %0"				"\n\t"
			"sv.q C110, 16 + %0"	"\n\t"
			"sv.q C120, 32 + %0"	"\n\t"
			"sv.q C130, 48 + %0"	"\n\t"
		: "=m" (*matrix)
		);
	} else {
		__asm__ volatile (
			"usv.q C100, %0"			"\n\t"
			"usv.q C110, 16 + %0"	"\n\t"
			"usv.q C120, 32 + %0"	"\n\t"
			"usv.q C130, 48 + %0"	"\n\t"
		: "=m" (*matrix)
		);
	}
}

void MATRIXFASTCALL MatrixIdentity	(float *matrix) //============== TODO
{
//	memset (matrix, 0, sizeof(float)*16);
//	matrix[0] = matrix[5] = matrix[10] = matrix[15] = 1.f;

	__asm__ volatile ("vmidt.q M000\n\t");

// check if matrix is 16 byte aligned
// as sv.q(7/2) is twice as fast as usv.q(14/4)			(pitch/latency)
// http://lukasz.dk/mirror/forums.ps2dev.org/viewtopic35a4.html?t=6929
	if (((u32)matrix&0xF)==0){
		__asm__ volatile (
			"sv.q C000, %0"					"\n\t"
			"sv.q C010, 16 + %0"		"\n\t"
			"sv.q C020, 32 + %0"		"\n\t"
			"sv.q C030, 48 + %0"		"\n\t"
		: "=m" (*matrix)
		);
	} else {
		__asm__ volatile (
			"usv.q C000, %0"				"\n\t"
			"usv.q C010, 16 + %0"		"\n\t"
			"usv.q C020, 32 + %0"		"\n\t"
			"usv.q C030, 48 + %0"		"\n\t"
		: "=m" (*matrix)
		);
	}
}

float MATRIXFASTCALL MatrixGetMultipliedIndex (int index, float *matrix, float *rightMatrix)
{
	int iMod = index%4, iDiv = (index>>2)<<2;

	return	(matrix[iMod  ]*rightMatrix[iDiv  ])+(matrix[iMod+ 4]*rightMatrix[iDiv+1])+
			(matrix[iMod+8]*rightMatrix[iDiv+2])+(matrix[iMod+12]*rightMatrix[iDiv+3]);
}

void MATRIXFASTCALL MatrixSet (float *matrix, int x, int y, float value)	// TODO
{
	matrix [x+(y<<2)] = value;
}

void MATRIXFASTCALL MatrixCopy (float* matrixDST, const float* matrixSRC)
{
//	memcpy ((void*)matrixDST, matrixSRC, sizeof(float)*16);

// i do believe this is a lot faster the mem* fncs.
/*
	if (((u32)matrixSRC&0xF)==0){
		__asm__ volatile (
			"lv.q C000, %0"							"\n\t"
			"lv.q C010, 16 + %0"				"\n\t"
			"lv.q C020, 32 + %0"				"\n\t"
			"lv.q C030, 48 + %0"				"\n\t"
		:: "m" (matrixSRC)
		);
	} else {*/
	
	// lv = 1, ulv = 2
	// checking for alignment takes another cycle so it won't help
		__asm__ volatile (
			"ulv.q C000, %0"						"\n\t"
			"ulv.q C010, 16 + %0"				"\n\t"
			"ulv.q C020, 32 + %0"				"\n\t"
			"ulv.q C030, 48 + %0"				"\n\t"
		:: "m" (*matrixSRC)
		);
//	}
	
	if (((u32)matrixDST&0xF)==0){
		__asm__ volatile (
			"sv.q C000, %0"							"\n\t"
			"sv.q C010, 16 + %0"				"\n\t"
			"sv.q C020, 32 + %0"				"\n\t"
			"sv.q C030, 48 + %0"				"\n\t"
		: "=m" (*matrixDST)
		);
	} else {
		__asm__ volatile (
			"usv.q C000, %0"						"\n\t"
			"usv.q C010, 16 + %0"				"\n\t"
			"usv.q C020, 32 + %0"				"\n\t"
			"usv.q C030, 48 + %0"				"\n\t"
		: "=m" (*matrixDST)
		);
	}

	/*
		__asm__ volatile (
			"ulv.q C000, %1"						"\n\t"
			"ulv.q C010, 16 + %1"				"\n\t"
			"ulv.q C020, 32 + %1"				"\n\t"
			"ulv.q C030, 48 + %1"				"\n\t"

			"usv.q C000, %0"						"\n\t"
			"usv.q C010, 16 + %0"				"\n\t"
			"usv.q C020, 32 + %0"				"\n\t"
			"usv.q C030, 48 + %0"				"\n\t"
		: "=m" (*matrixDST) : "m"(*matrixSRC)
		);
*/
}

int MATRIXFASTCALL MatrixCompare (const float* matrixDST, const float* matrixSRC)
{
	return memcmp((void*)matrixDST, matrixSRC, sizeof(float)*16);
}


void MatrixStackInit (MatrixStack *stack)
{
	stack->matrix	= NULL;
	stack->position	= 0;
	stack->size		= 0;
}

void MatrixStackSetMaxSize (MatrixStack *stack, int size)
{
	int i = 0;

	stack->size = size;

	if (stack->matrix == NULL)
	{
		stack->matrix = (float*) malloc (stack->size*16*sizeof(float));
	}
	else
	{
		free (stack->matrix);
		stack->matrix = (float*) malloc (stack->size*16*sizeof(float));
	}

	for (i = 0; i < stack->size; i++)
	{
		MatrixInit (&stack->matrix[i*16]);
	}

	stack->size--;
}


void MatrixStackSetStackPosition (MatrixStack *stack, int pos)
{
	stack->position += pos;

	if (stack->position < 0)
		stack->position = 0;
	else if (stack->position > stack->size)	
		stack->position = stack->size;
}

void MatrixStackPushMatrix (MatrixStack *stack, const float *ptr)
{
	MatrixCopy (&stack->matrix[stack->position*16], ptr);

	MatrixStackSetStackPosition (stack, 1);
}

float * MatrixStackPopMatrix (MatrixStack *stack, int size)
{
	MatrixStackSetStackPosition(stack, -size);

	return &stack->matrix[stack->position*16];
}

float * MatrixStackGetPos (MatrixStack *stack, int pos)
{
	return &stack->matrix[pos*16];
}

float * MatrixStackGet (MatrixStack *stack)
{
	return &stack->matrix[stack->position*16];
}

void MatrixStackLoadMatrix (MatrixStack *stack, int pos, const float *ptr)
{
	MatrixCopy (&stack->matrix[pos*16], ptr);
}

float Vector3Dot(const float *a, const float *b) 
{
//	return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];

	float rturn;

	__asm__ volatile (
		"lv.s S000, %1"					"\n\t"
		"lv.s S001, 4 + %1"					"\n\t"
		"lv.s S002, 8 + %1"					"\n\t"

		"lv.s S010, %2"					"\n\t"
		"lv.s S011, 4 + %2"					"\n\t"
		"lv.s S012, 8 + %2"					"\n\t"

		"vdot.t S020, C000, C010"		"\n\t"

		"sv.s S020, %0"							"\n\t"

	: "=m" (rturn) : "m"(*a), "m"(*b)
	);

	return rturn;
}

float Vector3Length(const float *a)
{/*
	float lengthSquared = Vector3Dot(a,a);
	float length = sqrt(lengthSquared);
	return length;*/

	float rturn;

	__asm__ volatile (
		"lv.s S000, 0 + %1"					"\n\t"
		"lv.s S001, 4 + %1"					"\n\t"
		"lv.s S002, 8 + %1"					"\n\t"

		"vdot.t S010, C000, C000"		"\n\t"
		"vsqrt.s S010, S000"				"\n\t"

		"sv.s S020, %0"		"\n\t"

	:"=m"(rturn) : "m"(*a)
	);

	return rturn;
}

void Vector3Add(float *dst, const float *src)
{/*
	dst[0] += src[0];
	dst[1] += src[1];
	dst[2] += src[2];*/

	__asm__ volatile (
		"lv.s S000, 0 + %1"		"\n\t"
		"lv.s S001, 4 + %1"		"\n\t"
		"lv.s S002, 8 + %1"		"\n\t"

		"lv.s S010, 0 + %0"		"\n\t"
		"lv.s S011, 4 + %0"		"\n\t"
		"lv.s S012, 8 + %0"		"\n\t"

		"vadd.t C020, C010, C000"		"\n\t"

		"sv.s S020, 0 + %0"		"\n\t"
		"sv.s S021, 4 + %0"		"\n\t"
		"sv.s S022, 8 + %0"		"\n\t"

	:"+m"(*dst) : "m"(*src)
	);

}

void Vector3Scale(float *dst, const float scale)
{/*
	dst[0] *= scale;
	dst[1] *= scale;
	dst[2] *= scale;*/

	__asm__ volatile (
		"lv.s S000, %1"		"\n\t"

		"lv.s S010, 0 + %0"		"\n\t"
		"lv.s S011, 4 + %0"		"\n\t"
		"lv.s S012, 8 + %0"		"\n\t"

		"vscl.t C020, C010, S000"		"\n\t"

		"sv.s S020, 0 + %0"		"\n\t"
		"sv.s S021, 4 + %0"		"\n\t"
		"sv.s S022, 8 + %0"		"\n\t"

	:"+m"(*dst) : "m"(scale)
	);

}

void Vector3Copy(float *dst, const float *src)
{/*
	dst[0] = src[0];
	dst[1] = src[1];
	dst[2] = src[2];*/
	
	__asm__ volatile (
		"lv.s S000, 0 + %1"		"\n\t"
		"lv.s S001, 4 + %1"		"\n\t"
		"lv.s S002, 8 + %1"		"\n\t"

		"sv.s S000, 0 + %0"		"\n\t"
		"sv.s S001, 4 + %0"		"\n\t"
		"sv.s S002, 8 + %0"		"\n\t"

	:"=m"(*dst) : "m"(*src)
	);

}

void Vector3Normalize(float *dst)
{
	float length = Vector3Length(dst);
	Vector3Scale(dst,1.0f/length);
}

void Vector4Copy(float *dst, const float *src)
{/*
	dst[0] = src[0];
	dst[1] = src[1];
	dst[2] = src[2];
	dst[3] = src[3];*/

	__asm__ volatile (
		"ulv.q C000, %1"		"\n\t"
		"usv.q C000, %0"		"\n\t"
	:"=m"(*dst) : "m"(*src)
	);

/*
	__asm__ volatile (
		"lv.s S000, 0 + %1"		"\n\t"
		"lv.s S001, 4 + %1"		"\n\t"
		"lv.s S002, 8 + %1"		"\n\t"
		"lv.s S003, 12 + %1"		"\n\t"

		"sv.s S000, 0 + %0"		"\n\t"
		"sv.s S001, 4 + %0"		"\n\t"
		"sv.s S002, 8 + %0"		"\n\t"
		"sv.s S003, 12 + %0"		"\n\t"

	:"=m"(*dst) : "m"(*src)
	);
*/

}
