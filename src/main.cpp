#include <Arduino.h>
#include "testData.h"
#include "arrayProcessing.h"
#include <arduinoFFT.h>

#define bufferSize 512
#define singleSize (bufferSize / 2)

double pytData[bufferSize*2];
double copyData[bufferSize*2];

double acclPyt[bufferSize];
double gyroPyt[bufferSize];

double acclSingleFFT[singleSize];
double gyroSingleFFT[singleSize];

// FFT reel og imaginær array
double realFFT[bufferSize];
double imagFFT[bufferSize];

// Aktivitets typer (som numbereret liste via enum)
typedef enum {
	UNKNOWN, // 0 - Ukendt aktivitet
	RUN_WALK, // 1 - Løb eller gang aktivitet
	RUN, // 2 - Løb
	WALK, // 3 - Gang
	BIKE, // 4 - Cykling med ukendt hastighed
	BIKE_SLOW, // 5 - Langsom cykling
	BIKE_FAST // 6 - Hurtig cykling
} activityTypes;

// Samplerate i Hz
#define fs 100
// Frekvens i Hz hvor over og under sammen skal sammenlignes
#define sumFreq 8
uint16_t fftIndexSummed = ((bufferSize / fs) * sumFreq) + 1;

// Peak detektion threshold
#define acclPeakThreshold 0.65
#define gyroPeakThreshold 0.65 
// Peak detektion timeout
#define acclPeakTimeout 60
#define gyroPeakTimeout 15
// Rotationer per minut for hurtig cykling
#define fastBikeSPM 155
// Skridt per minut for løb
#define runSPM 130


arduinoFFT FFT = arduinoFFT(); // FFT klasse

// Beregn Pythagoras bevægelsesvektor fra x, y og z
double calculatePythagoras(int16_t x, int16_t y, int16_t z) {
	double pythagoras = (pow(x,2) + pow(y,2) + pow(z,2));
	pythagoras = sqrt(pythagoras);
	return pythagoras;
}

// Udfør FFT på data og kopier absolut single sided FFT til absFFTout array (output array (absFFTout) længde er (arrayLength/2)+1)
void getAbsoluteSingleFFT(double *rawDataIn, double *absFFTout, uint16_t arrayLength) {
	// Kopier data til FFT array
	copyArray(rawDataIn, realFFT, arrayLength); 
	// Reset alle værdier i imagFFT (nødvendigt ifølge arduinoFFT bibliotek)
	setArrayTo(imagFFT, arrayLength, 0); 
	// Udfør FFT
	FFT.Compute(realFFT, imagFFT, arrayLength, FFT_FORWARD);
	// Beregn absolute værdi af FFT 
	absComplexArray(realFFT, imagFFT, arrayLength); 
	// Kopier data til FFT array
	copyArray(realFFT, absFFTout, (arrayLength / 2)); 
}

// Estimer aktivitet baseret på summen af frekvensindhold over og under hzIndex
uint8_t estimateActivity(double *acclSumBelow, double *acclSumAbove, double *gyroSumBelow, double *gyroSumAbove) {
	if ((*acclSumAbove > *acclSumBelow) && (*gyroSumAbove > *gyroSumBelow)) {
		#ifdef printFunc_estimateActivity
			Serial.println("estimateActivity(), Activity is RUN_WALK");
		#endif
		return RUN_WALK;
	} else if ((*acclSumAbove < *acclSumBelow) && (*gyroSumAbove < *gyroSumBelow)) {
		#ifdef printFunc_estimateActivity
			Serial.println("estimateActivity(), Activity is BIKE");
		#endif
		return BIKE;
	}
	#ifdef printFunc_estimateActivity
		Serial.println("estimateActivity(), Activity is UNKNOWN");
	#endif
	return UNKNOWN;
}

// Korriger tidligere estimeret aktivitet
uint8_t specifyActivity(uint8_t activity, int16_t peaks) {
	#ifdef printFunc_specifyActivity
		Serial.print("specifyActivity(), Specified activity is: ");
	#endif
	switch (activity) {
		case BIKE: {
			float spinsPerMin = ((float)peaks / 5.0f) * 60.0f;
			#ifdef printFunc_specifyActivity
				Serial.print("Spins per min is: ");
				Serial.println(spinsPerMin);
			#endif	
			if (spinsPerMin > fastBikeSPM) {
				#ifdef printFunc_specifyActivity
					Serial.println("FAST BIKE");
				#endif
				activity = BIKE_FAST;
			} else {
				#ifdef printFunc_specifyActivity
					Serial.println("SLOW BIKE");
				#endif				
				activity = BIKE_SLOW;
			}
		} break;
		case RUN_WALK: {
			float stepsPerMin = (((float)peaks * 2.0f) / 5.0f) * 60.0f;
			#ifdef printFunc_specifyActivity
				Serial.print("Steps per min is: ");
				Serial.println(stepsPerMin);
			#endif	
			if (stepsPerMin > runSPM) {
				#ifdef printFunc_specifyActivity
					Serial.println("RUN");
				#endif				
				activity = RUN;
			} else {
				#ifdef printFunc_specifyActivity
					Serial.println("WALK");
				#endif				
				activity = WALK;
			}
		} break;
	}
	return activity;
}

void setup() {
	Serial.begin(115200);
	Serial.println("Starting compare!");


	Serial.println("-----------------------------");
	Serial.println("Comparing MATLAB Pythagoras and calculated Pythagoras...");
	Serial.println("-----------------------------");

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


	Serial.println("-----------------------------");
	Serial.println("Tjekking copy array function...");
	Serial.println("-----------------------------");

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


	Serial.println("-----------------------------");
	Serial.println("Tjekking min/max...");
	Serial.println("-----------------------------");

	uint16_t dataIndex = 0;
	for (uint16_t i = 0; i < bufferSize; i++) {
		acclPyt[i] = pytData[dataIndex];
		gyroPyt[i] = pytData[dataIndex + 1];
		dataIndex = dataIndex + 2;
	}


	double *arryPointer = acclPyt;
	Serial.print("Value at acclPyt + 1: ");
	Serial.println(*(arryPointer+1));

	double matlabAcclMax = aMinMax[1];
	double matlabGyroMax = aMinMax[3];

	// Find max i accelerometerdata
	double adataMax = maxInArray(acclPyt, bufferSize);
	// Find max i gyroskop data
	double gdataMax = maxInArray(gyroPyt, bufferSize);

	Serial.print("MATLAB max: ");
	Serial.print(matlabAcclMax);
	Serial.print(" ");
	Serial.println(matlabGyroMax);	

	Serial.print("Found max: ");
	Serial.print(adataMax);
	Serial.print(" ");
	Serial.println(gdataMax);	

	if ((int16_t)matlabAcclMax != (int16_t)adataMax) {
		Serial.println("Accl max not equal!");
	}
	if ((int16_t)matlabGyroMax != (int16_t)gdataMax) {
		Serial.println("Gyro max not equal!");
	}
	Serial.println("Done");


	Serial.println("-----------------------------");
	Serial.println("Tester normalisering af array...");
	Serial.println("-----------------------------");

	normalizeArray(acclPyt, bufferSize, adataMax);
	normalizeArray(gyroPyt, bufferSize, gdataMax);

	isSimilar(acclNorm, acclPyt, bufferSize);
	isSimilar(gyroNorm, gyroPyt, bufferSize);
	Serial.println("Done");


	Serial.println("-----------------------------");
	Serial.println("Tester FFT konvertering...");
	Serial.println("-----------------------------");

	getAbsoluteSingleFFT(acclPyt, acclSingleFFT, bufferSize);
	getAbsoluteSingleFFT(gyroPyt, gyroSingleFFT, bufferSize);

	setArrayTo(acclSingleFFT, 5, 0); 
	setArrayTo(gyroSingleFFT, 5, 0); 

	isSimilar(acclFFT, acclSingleFFT, singleSize);
	isSimilar(gyroFFT, gyroSingleFFT, singleSize);
	Serial.println("Done");


	Serial.println("-----------------------------");
	Serial.println("Tester FFT sum over og under 8 Hz...");
	Serial.println("-----------------------------");

	// Find sum under hzIndex og over hzIndex
	double acclBelowSum = arraySum(acclSingleFFT, 0, fftIndexSummed);
	double acclAboveSum = arraySum(acclSingleFFT, fftIndexSummed, singleSize);
	double gyroBelowSum = arraySum(gyroSingleFFT, 0, fftIndexSummed);
	double gyroAboveSum = arraySum(gyroSingleFFT, fftIndexSummed, singleSize);

	double calculatedFFTsum[] = {
		acclBelowSum, acclAboveSum, gyroBelowSum, gyroAboveSum
	};

	Serial.print("MATLAB summes (acclBelow, acclAbove, gyroBelow, gyroAbove): ");
	Serial.print(summedFFT[0]);
	Serial.print(" ");
	Serial.print(summedFFT[1]);
	Serial.print(" ");
	Serial.print(summedFFT[2]);
	Serial.print(" ");
	Serial.println(summedFFT[3]);	

	Serial.print("Found summes (acclBelow, acclAbove, gyroBelow, gyroAbove): ");
	Serial.print(calculatedFFTsum[0]);
	Serial.print(" ");
	Serial.print(calculatedFFTsum[1]);
	Serial.print(" ");
	Serial.print(calculatedFFTsum[2]);
	Serial.print(" ");
	Serial.println(calculatedFFTsum[3]);	

	isSimilar(summedFFT, calculatedFFTsum, 4);
	Serial.println("Done");


	Serial.println("-----------------------------");
	Serial.println("Tjekker aktivitet...");
	Serial.println("-----------------------------");

	// Gæt aktivitet baseret på FFT
	uint8_t activity = estimateActivity(&calculatedFFTsum[0], &calculatedFFTsum[1], &calculatedFFTsum[2], &calculatedFFTsum[3]);
	Serial.println("Done");


	Serial.println("-----------------------------");
	Serial.println("Tjekker find peaks...");
	Serial.println("-----------------------------");

	uint16_t peakCount = 0;
	// Find peaks i data
	switch (activity) {
		case RUN_WALK: {
			peakCount = findPeaksInArray(acclPyt, bufferSize, acclPeakThreshold, acclPeakTimeout);
		} break;
		case BIKE: {
			peakCount = findPeaksInArray(gyroPyt, bufferSize, gyroPeakThreshold, gyroPeakTimeout);
		} break;
	}
	Serial.print("MATLAB peaks: ");
	Serial.println(matlabPeaks);
	Serial.print("Peaks fundet: ");
	Serial.println(peakCount);

	if (matlabPeaks != peakCount) {
		Serial.println("Peak detektion ukorrekt!");
	} else {
		Serial.println("Peak detektion korrekt!");
	}
	Serial.println("Done");


	Serial.println("-----------------------------");
	Serial.println("Tjekker aktivitetSpeficier...");
	Serial.println("-----------------------------");

	// Korriger aktivitet
	activity = specifyActivity(activity, peakCount);
	
	Serial.println("Done");

}

void loop() {


}