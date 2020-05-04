#include <Arduino.h>
#include "testData.h"

#define bufferSize 512

double pytData[bufferSize*2];
double copyData[bufferSize*2];

double acclPyt[bufferSize];
double gyroPyt[bufferSize];

// Beregn Pythagoras bevægelsesvektor fra x, y og z
double calculatePythagoras(int16_t x, int16_t y, int16_t z) {
	double pythagoras = (pow(x,2) + pow(y,2) + pow(z,2));
	pythagoras = sqrt(pythagoras);
	return pythagoras;
}

// Kopier double array til double array
void copyArray(double *fromArray, double *toArray, int16_t arrayLength) {
	for (uint16_t i = 0; i < arrayLength; i++) {
		*(toArray+i) = *(fromArray+i);
	}
}

// Finder maksimal værdi i array
double maxInArray(double *buffPointer, uint16_t buffSize) {
	double maxValue = *(buffPointer);
	#ifdef printFunc_maxInArray
		uint16_t maxIndex = 0;
	#endif
	for (uint16_t i = 1; i < buffSize; i++) {
		if (maxValue < *(buffPointer+i)) {
			maxValue = *(buffPointer+i);
			#ifdef printFunc_maxInArray
				maxIndex = i;
			#endif
		}
	}
	#ifdef printFunc_maxInArray
		Serial.print("maxInArray(), max is: ");
		Serial.print(maxValue);
		Serial.print(" at index: ");
		Serial.println(maxIndex);
	#endif
	return maxValue;	
}

// Finder minimal værdi i array
double minInArray(double *buffPointer, uint16_t buffSize) {
	double minValue = *(buffPointer);
	Serial.println(minValue);
	#ifdef printFunc_maxInArray
		uint16_t minIndex = 0;
	#endif
	for (uint16_t i = 1; i < buffSize; i++) {
		if (minValue > *(buffPointer+i)) {
			Serial.print(minValue);
			Serial.print(" > ");
			Serial.println(*(buffPointer+i));
			minValue = *(buffPointer+i);
			if ((uint16_t)minValue == 0) {
				Serial.print("0 at: ");
				Serial.println(i);
			}
			#ifdef printFunc_maxInArray
				minIndex = i;
			#endif
		}
	}
	#ifdef printFunc_minInArray
		Serial.print("minInArray(), min is: ");
		Serial.print(minValue);
		Serial.print(" at index: ");
		Serial.println(minIndex);
	#endif
	return minValue;
}

// Bereng summen af alle datapunkter i et array fra startIndex til endIndex. endIndex bør ikke overstige størrelsen af arrayet. startindex er inklusiv, endIndex er eksklusiv
double arraySum(double *arrayPointer, uint16_t startIndex, uint16_t endIndex) {
	double sumOfArray = 0;
	for (uint16_t i = startIndex; i < endIndex; i++) {
		sumOfArray = sumOfArray + *(arrayPointer+i);
	}
	#ifdef printFunc_arraySum
		Serial.print("arraySum(), Finding sum, with start/stop and sum: ");
		Serial.print(startIndex);
		Serial.print(" ");
		Serial.print(endIndex);
		Serial.print(" ");
		Serial.println(sumOfArray);
	#endif
	return sumOfArray;
}


void setup() {
	Serial.begin(115200);
	Serial.println("Starting compare!");

	Serial.println("Comparing MATLAB Pythagoras and calculated Pythagoras...");
	uint16_t pytIndex = 0;
	for (uint16_t i = 0; i < bufferSize*6; i = i + 6) {
		pytData[pytIndex] = calculatePythagoras(rawDataOut[i], rawDataOut[i+1], rawDataOut[i+2]);
		pytData[pytIndex + 1] = calculatePythagoras(rawDataOut[i+3], rawDataOut[i+4], rawDataOut[i+5]);
		pytIndex = pytIndex + 2;
	}
	for (uint16_t i = 0; i < bufferSize*2; i++) {
		double matlab = (int16_t)pytDataOut[i];
		double calculated = (int16_t)pytData[i];
		if (matlab != calculated) {
			Serial.print("Not equal! At: ");
			Serial.print(i);
			Serial.print(" matlab vs calculated: ");
			Serial.print(matlab);
			Serial.print(" ");
			Serial.println(calculated);
		}
	}
	Serial.println("Done!");


	Serial.println("Tjekking copy array function...");
	copyArray(pytData, copyData, bufferSize*2);
	for (uint16_t i = 0; i < bufferSize*2; i++) {
		double original = (int16_t)pytData[i];
		double copied = (int16_t)copyData[i];
		if (original != copied) {
			Serial.print("Not equal! At: ");
			Serial.print(i);
			Serial.print(" original vs copied: ");
			Serial.print(original);
			Serial.print(" ");
			Serial.println(copied);
		}
	}
	Serial.println("Done!");


	Serial.println("Tjekking min/max...");
	uint16_t dataIndex = 0;
	for (uint16_t i = 0; i < bufferSize; i = i + 6) {
		acclPyt[i] = pytData[dataIndex];
		gyroPyt[i] = pytData[dataIndex + 1];
		dataIndex = dataIndex + 2;
	}


	double *arryPointer = acclPyt;
	Serial.print("Value at acclPyt + 1: ");
	Serial.println(*(arryPointer+1));

	double matlabAcclMin = aMinMax[0];
	double matlabAcclMax = aMinMax[2];
	double matlabGyroMin = aMinMax[2];
	double matlabGyroMax = aMinMax[3];

	Serial.print("MATLAB min/max: ");
	Serial.print(matlabAcclMin);
	Serial.print(" ");
	Serial.print(matlabAcclMax);
	Serial.print(" ");
	Serial.print(matlabGyroMin);
	Serial.print(" ");
	Serial.println(matlabGyroMax);	

	// Find min og max i accelerometerdata
	double adataMax = maxInArray(acclPyt, bufferSize);
	double adataMin = minInArray(acclPyt, bufferSize);
	// Find min og max i gyroskop data
	double gdataMax = maxInArray(gyroPyt, bufferSize);
	double gdataMin = minInArray(gyroPyt, bufferSize);

	Serial.print("Found min/max: ");
	Serial.print(adataMin);
	Serial.print(" ");
	Serial.print(adataMax);
	Serial.print(" ");
	Serial.print(gdataMin);
	Serial.print(" ");
	Serial.println(gdataMax);	

	if ((int16_t)matlabAcclMin != (int16_t)adataMin) {
		Serial.println("Accl min not equal!");
	}
	if ((int16_t)matlabAcclMax != (int16_t)adataMax) {
		Serial.println("Accl max not equal!");
	}
	if ((int16_t)matlabGyroMin != (int16_t)gdataMin) {
		Serial.println("Gyro min not equal!");
	}
	if ((int16_t)matlabGyroMax != (int16_t)gdataMax) {
		Serial.println("Gyro max not equal!");
	}

	Serial.println("Done");



}

void loop() {


}