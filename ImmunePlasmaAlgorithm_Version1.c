#include <time.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

//Include user defined header file or files
#include "timer.h"
//Include user defined header file or files

//Define PI value
#define PI 3.14159265358979323846
//Define PI value

//Define number of indidivuals and parameters
#define NUMBEROFTHREADS 1
#define MIGRATIONPERIOD 10
#define NUMBEROFPARAMETERS 4
#define OBJECTIVEFUNCTION calculatePressureVesselCost
//Define number of indidivuals and parameters

//Define number of individuals and donors
#define NUMBEROFDONORS 1
#define NUMBEROFRECEIVERS 1 
#define NUMBEROFINDIVIDUALS 100
#define NUMBEROFINDIVIDUALSINTHREAD ( NUMBEROFINDIVIDUALS / NUMBEROFTHREADS )
//Define number of individuals and donors

//Define number of cycles
#define NUMBEROFCYCLES 100
#define MAXIMUMFITNESSEVALUATIONS 100000
//Define number of cycles

//Semaphores to control accessing emigrant individuals
sem_t* emigrantControlSemaphores;
//Semaphores to control accessing emigrant individuals
double lowerBounds[] = { 0.0, 0.0, 10.0, 10.0 };  // Ts, Th, R, L alt sýnýrlarý
double upperBounds[] = { 99.0, 99.0, 200.0, 200.0 };  // Ts, Th, R, L üst sýnýrlarý
double calculatePressureVesselCost(double* x);

//Global variables used to store information algorithm
double* individuals[ NUMBEROFTHREADS ];
int* indexesOfDonors[ NUMBEROFTHREADS ];
int* indexesOfReceivers[ NUMBEROFTHREADS ];
double* objectiveFunctionValues[ NUMBEROFTHREADS ];
//Global variables used to store information algorithm

//Global variables used to store information about emigrant individuals
double* emigrantIndividuals[ ( ( NUMBEROFCYCLES / MIGRATIONPERIOD ) ) * NUMBEROFTHREADS ];
double* emigrantObjectiveFunctionValues[ ( ( NUMBEROFCYCLES / MIGRATIONPERIOD ) ) * NUMBEROFTHREADS ];
//Global variables used to store information about emignrat individuals

//Global variables used to store best objective function values
long* evaluationIndexes[ NUMBEROFTHREADS ]; 
double* bestObjectiveFunctionValues[ NUMBEROFTHREADS ];
double* bestObjectiveFunctionValuesOfEvaluations[ NUMBEROFTHREADS ];
//Global variables used to store best objective function values

void initializeIndividual( double* individual );
void initializeIndividuals( double* individuals );
void calculateObjectiveFunctionValue( double* individual, double* objectiveFunctionValue, double ( *calculateObjectiveFunction )( double* individual ) );
void calculateObjectiveFunctionValues( double* individuals, double* objectiveFunctionValues, double ( *calculateObjectiveFunction )( double* individual ) );

int getChangedParameterIndex();
int getNeighborFoodSourceIndex( int foodSourceIndex );
double getBestObjectiveFunctionValue( long threadRank );

void printIndividuals();
void printBestObjectiveFunctionValue();
void printBestObjectiveFunctionValuesToFile( char* runIndex );
void printElapsedTimeToFile( double elapsedTime, char* runIndex );

//Function prototypes for emigrant food source
void getEmigrantFoodSource( long threadRank, long migrationRank );
void setEmigrantFoodSource( long threadRank, long neighborThreadRank, long migrationRank );
//Function prototypes for emigrant food source

//Function prototypes for algorithm
void immuneSystemPhase( long threadRank );
void convalescentTreatmentPhase( long threadRank );
void getDonorAndReceiverIndexes( long threadRank );
//Function prototypes for algorithm

//Function prototypes for threads
void* phases( void* rank );
//Function prototypes for threads

int main( int argc, char* argv [] ){
 long threadIndex;
 pthread_t* threadHandles;
 double startTime, finishTime;

 threadHandles = ( pthread_t*)malloc( NUMBEROFTHREADS * sizeof( pthread_t ) );
 emigrantControlSemaphores = ( sem_t* )malloc( ( ( NUMBEROFCYCLES / MIGRATIONPERIOD ) ) * NUMBEROFTHREADS * sizeof( sem_t ) );

 //Allocate memory spaces for emigrant food sources
 for( int emigrantIndex = 0; emigrantIndex < ( ( NUMBEROFCYCLES / MIGRATIONPERIOD ) ) * NUMBEROFTHREADS; emigrantIndex ++ ){
  double* emigrantObjectiveFunctionValue = ( double* )malloc( sizeof( double ) );
  double* emigrantIndividual = ( double* )malloc( NUMBEROFPARAMETERS * sizeof( double ) );
  
  emigrantIndividuals[ emigrantIndex ] = emigrantIndividual;
  emigrantObjectiveFunctionValues[ emigrantIndex ] = emigrantObjectiveFunctionValue;
 }
 //Allocate memory spaces for emigrant food sources
 
 //Allocate memory spaces for best food sources
 for( threadIndex = 0; threadIndex < NUMBEROFTHREADS; threadIndex ++ ){
  double* bestObjectiveFunctionValue = ( double* )malloc( sizeof( double ) );
  bestObjectiveFunctionValues[ threadIndex ] = bestObjectiveFunctionValue;
  double* bestObjectiveFunctionValuesOfEvaluationsInThread = ( double* )malloc( MAXIMUMFITNESSEVALUATIONS * sizeof( double ) );
  bestObjectiveFunctionValuesOfEvaluations[ threadIndex ] = bestObjectiveFunctionValuesOfEvaluationsInThread;
 }
 //Allocate memory spaces for best food sources

 //Initialize semaphores
 for( int semaphoreIndex = 0; semaphoreIndex < ( ( NUMBEROFCYCLES / MIGRATIONPERIOD ) ) * NUMBEROFTHREADS; semaphoreIndex ++ ){
  sem_init( &emigrantControlSemaphores[ semaphoreIndex ], 0, 0 );
 }
 //Initialize semaphores

 GET_TIME( startTime );
 for( threadIndex = 0; threadIndex < NUMBEROFTHREADS; threadIndex ++ ){
  pthread_create( &threadHandles[ threadIndex ], NULL, phases, ( void* ) threadIndex ); 
 } 
 for( threadIndex = 0; threadIndex < NUMBEROFTHREADS; threadIndex ++ ){
  pthread_join( threadHandles[ threadIndex ], NULL );
 }
 GET_TIME( finishTime );

 //Destroy semaphores
 for( int semaphoreIndex = 0; semaphoreIndex < ( ( NUMBEROFCYCLES / MIGRATIONPERIOD ) ) * NUMBEROFTHREADS; semaphoreIndex ++ ){
  sem_destroy( &emigrantControlSemaphores[ semaphoreIndex ] );
 }
 //Destroy semaphores

 printf( "NumberOfThreads:%d\n", NUMBEROFTHREADS );
 printf( "NumberOfParameters:%d\n", NUMBEROFPARAMETERS );
 printf( "NumberOfIndividuals:%d\n", NUMBEROFINDIVIDUALS );
 printf( "ElapsedTime:%lf second\n", finishTime - startTime );
 
 //printIndividuals();
 printBestObjectiveFunctionValue( argv[ 1 ] );
 printBestObjectiveFunctionValuesToFile( argv[ 1 ] );
 printElapsedTimeToFile( finishTime - startTime, argv[ 1 ] );

 //Free allocated spaces
 for( threadIndex = 0; threadIndex < NUMBEROFTHREADS; threadIndex ++ ){
  free( individuals[ threadIndex ] );
  free( evaluationIndexes[ threadIndex ] );
  free( objectiveFunctionValues[ threadIndex ] );
  free( bestObjectiveFunctionValues[ threadIndex ] );
  free( bestObjectiveFunctionValuesOfEvaluations[ threadIndex ] );
 }
 for( int emigrantIndex = 0; emigrantIndex < ( ( NUMBEROFCYCLES / MIGRATIONPERIOD ) ) * NUMBEROFTHREADS; emigrantIndex ++ ){
  free( emigrantIndividuals[ emigrantIndex ] );
  free( emigrantObjectiveFunctionValues[ emigrantIndex ] );
 }
 free( threadHandles );
 free( emigrantControlSemaphores );
 //Free allocated spaces

 return 0;
}

void initializeIndividual(double* individual) {
    for (int parameterIndex = 0; parameterIndex < NUMBEROFPARAMETERS; parameterIndex++) {
        individual[parameterIndex] = lowerBounds[parameterIndex] +
            (rand() / (RAND_MAX + 1.0)) * (upperBounds[parameterIndex] - lowerBounds[parameterIndex]);
    }
}

void initializeIndividuals(double* individuals) {
    for (int individualIndex = 0; individualIndex < NUMBEROFINDIVIDUALSINTHREAD; individualIndex++) {
        double* individual = (double*)((char*)individuals + individualIndex * NUMBEROFPARAMETERS * sizeof(double));
        for (int parameterIndex = 0; parameterIndex < NUMBEROFPARAMETERS; parameterIndex++) {
            individual[parameterIndex] = lowerBounds[parameterIndex] +
                (rand() / (RAND_MAX + 1.0)) * (upperBounds[parameterIndex] - lowerBounds[parameterIndex]);
        }
    }
}

void calculateObjectiveFunctionValue( double* individual, double* objectiveFunctionValue, double ( *calculateObjectiveFunction )( double* individual ) ){
 ( *objectiveFunctionValue ) = calculateObjectiveFunction( individual );
}
void calculateObjectiveFunctionValues( double* individuals, double* objectiveFunctionValues, double ( *calculateObjectiveFunction )( double* individual ) ){
 for( int individualIndex = 0; individualIndex < NUMBEROFINDIVIDUALSINTHREAD; individualIndex ++ ){
  double* individual = ( double* )( ( char* )individuals + individualIndex * NUMBEROFPARAMETERS * sizeof( double ) );
  *( objectiveFunctionValues + individualIndex ) = calculateObjectiveFunction( individual );
 }
}

void printIndividuals(){
 for( int threadIndex = 0; threadIndex < NUMBEROFTHREADS; threadIndex ++ ){
  double* individualsInThread = individuals[ threadIndex ];
  double* objectiveFunctionValuesInThread = objectiveFunctionValues[ threadIndex ];
  for( int individualIndex = 0; individualIndex < NUMBEROFINDIVIDUALSINTHREAD; individualIndex ++ ){
   double* individual = ( double* )( ( char* )individualsInThread + individualIndex * NUMBEROFPARAMETERS * sizeof( double ) ); 
   for( int parameterIndex = 0; parameterIndex < 15; parameterIndex ++ ){
    printf( "%6.2f  ", *( individual + parameterIndex ) );
   }
   printf( "Obj:%.2e\n", *( objectiveFunctionValuesInThread + individualIndex ) );
  }
 }
}
void printBestObjectiveFunctionValue(){
 for( int threadIndex = 0; threadIndex < NUMBEROFTHREADS; threadIndex ++ ){
  double* bestObjectiveFunctionValue = bestObjectiveFunctionValues[ threadIndex ];
  printf( "Subcolony:%-4dBestObjectiveFunctionValue:%e\n", ( threadIndex + 1 ), *( bestObjectiveFunctionValue ) );
 }
}
void printBestObjectiveFunctionValuesToFile( char* runIndex ){
 char absolutePath[ 100 ];
 double bestObjectiveFunctionValue;
 FILE* bestObjectiveFunctionValuesFILE = NULL;
 double* bestObjectiveFunctionValuesOfEvaluationsInThread = NULL;
 double* temporaryBestObjectiveFunctionValuesOfEvaluationsInThread = NULL;
 sprintf( absolutePath, "./Results/Run%s.dat", runIndex );
 if( ( bestObjectiveFunctionValuesFILE = fopen( absolutePath, "w" ) ) != NULL ){
  for( int evaluationIndex = 0; evaluationIndex < MAXIMUMFITNESSEVALUATIONS; evaluationIndex ++ ){
   bestObjectiveFunctionValuesOfEvaluationsInThread = bestObjectiveFunctionValuesOfEvaluations[ 0 ];
   bestObjectiveFunctionValue = *( bestObjectiveFunctionValuesOfEvaluationsInThread + evaluationIndex );
   for( int threadIndex = 1; threadIndex < NUMBEROFTHREADS; threadIndex ++ ){
    temporaryBestObjectiveFunctionValuesOfEvaluationsInThread = bestObjectiveFunctionValuesOfEvaluations[ threadIndex ];
    if( bestObjectiveFunctionValue > *( temporaryBestObjectiveFunctionValuesOfEvaluationsInThread + evaluationIndex ) ){
     bestObjectiveFunctionValue = *( temporaryBestObjectiveFunctionValuesOfEvaluationsInThread + evaluationIndex );
    }
   }
   fprintf( bestObjectiveFunctionValuesFILE, "%e\n", bestObjectiveFunctionValue );
  }
  fclose( bestObjectiveFunctionValuesFILE );
 }
 else{
  fprintf( stderr, "An error has occured while writing to file...\n" );
  exit( EXIT_FAILURE );
 } 
}
void printElapsedTimeToFile( double elapsedTime, char* runIndex ){
 char absolutePath[ 80 ];
 FILE* elapsedTimeFILE = NULL;
 sprintf( absolutePath, "./Results/ElapsedTime%s.dat", runIndex );
 if( ( elapsedTimeFILE = fopen( absolutePath, "w" )) != NULL ){
  fprintf( elapsedTimeFILE, "%f\n", elapsedTime );
  fclose( elapsedTimeFILE );
 }
 else{
  fprintf( stderr, "An error has occured while writing to file...\n" );
  exit( EXIT_FAILURE );
 }
}

int getChangedParameterIndex(){
 return ( int )( ( ( 1.0 * rand() ) / ( RAND_MAX + 1.0 ) ) * NUMBEROFPARAMETERS );
}
int getBestIndividualIndex( long threadRank ){
 int bestIndividualIndex = 0;
 double* objectiveFunctionValuesInThread = objectiveFunctionValues[ threadRank ];
 //Get best individual index
 for( int individualIndex = 1; individualIndex < NUMBEROFINDIVIDUALSINTHREAD; individualIndex ++ ){
  if( *( objectiveFunctionValuesInThread + bestIndividualIndex ) > *( objectiveFunctionValuesInThread + individualIndex ) ){
   bestIndividualIndex = individualIndex;
  }
 }
 //Get best individual index
 return bestIndividualIndex;
}
int getRandomIndividualIndex( int individualIndex ){
 int randomIndividualIndex = ( int )( ( ( 1.0 * rand() ) / ( RAND_MAX + 1.0 ) ) * NUMBEROFINDIVIDUALSINTHREAD );
 while( randomIndividualIndex == individualIndex ){
  randomIndividualIndex = ( int )( ( ( 1.0 * rand() ) / ( RAND_MAX + 1.0 ) ) * NUMBEROFINDIVIDUALSINTHREAD );
 }
 return randomIndividualIndex;
}
double getBestObjectiveFunctionValue( long threadRank ){
 int bestIndividualIndex = 0;
 double* objectiveFunctionValuesInThread = objectiveFunctionValues[ threadRank ];
 //Get best individual index
 for( int individualIndex = 1; individualIndex < NUMBEROFINDIVIDUALSINTHREAD; individualIndex ++ ){
  if( *( objectiveFunctionValuesInThread + bestIndividualIndex ) > *( objectiveFunctionValuesInThread + individualIndex ) ){
   bestIndividualIndex = individualIndex;
  }
 }
 //Get best individual index
 return *( objectiveFunctionValuesInThread + bestIndividualIndex );
}

double calculateStepFunction( double* individual ){
 double firstDouble_Variable = 0.0;
 for( int parameterIndex = 0; parameterIndex < NUMBEROFPARAMETERS; parameterIndex ++ ){
  firstDouble_Variable = firstDouble_Variable + pow( floor( *( individual + parameterIndex ) + 0.5 ) , 2 ); 
 }
 return firstDouble_Variable;
}
double calculateAckleyFunction( double* individual ){
 double firstDouble_Variable = 0.0;
 double secondDouble_Variable = 0.0; 
 for( int parameterIndex = 0 ; parameterIndex < NUMBEROFPARAMETERS; parameterIndex ++ ){
  firstDouble_Variable = firstDouble_Variable + pow( *( individual + parameterIndex ) , 2 );
  secondDouble_Variable = secondDouble_Variable + cos( 2 * 3.14 * *( individual + parameterIndex ) );
 }
 return -20.0 * exp( -0.2 * sqrt( ( 1.0 / ( NUMBEROFPARAMETERS * 1.0 ) ) * firstDouble_Variable ) ) - exp( ( 1.0 / ( NUMBEROFPARAMETERS * 1.0 ) ) * secondDouble_Variable ) + 20.0 + exp( 1.0 );
}
double calculateSphereFunction( double* individual ){
 double sphereFunctionValue = 0.0;
 for( int parameterIndex = 0; parameterIndex < NUMBEROFPARAMETERS; parameterIndex ++ ){
  sphereFunctionValue = sphereFunctionValue + *( individual + parameterIndex ) * *( individual + parameterIndex ); 
 }
 return sphereFunctionValue;
}
double calculateQuarticFunction( double* individual ){
 double firstDouble_Variable = 0.0;
 for( int parameterIndex = 0; parameterIndex < NUMBEROFPARAMETERS; parameterIndex ++ ){
  firstDouble_Variable = firstDouble_Variable + ( parameterIndex + 1 ) * pow( *( individual + parameterIndex ), 4 ); 
 }
 return firstDouble_Variable + ( ( 1.0 * rand() ) / ( 1.0 * RAND_MAX ) );
}
double calculateSchwefelFunction( double* individual ){
 double firstDouble_Variable = 0.0;
 for( int parameterIndex = 0; parameterIndex < NUMBEROFPARAMETERS; parameterIndex ++ ){
  firstDouble_Variable = firstDouble_Variable + ( *( individual + parameterIndex ) * sin( sqrt( fabs( *( individual + parameterIndex ) ) ) ) );
 }
 return ( -1.0 ) * firstDouble_Variable;
}
double calculateGriewankFunction( double* individual ){
 double additivePartOfGriewankFunctionValue = 0.0;
 double multiplicativePartOfGriewankFunctionValue = 1.0;
 for( int parameterIndex = 0; parameterIndex < NUMBEROFPARAMETERS; parameterIndex ++ ){
  additivePartOfGriewankFunctionValue = additivePartOfGriewankFunctionValue + ( ( 1.0 / 4000.0 ) * ( *( individual + parameterIndex ) * *( individual + parameterIndex ) ) );
  multiplicativePartOfGriewankFunctionValue = multiplicativePartOfGriewankFunctionValue * ( cos( *( individual + parameterIndex ) / sqrt( parameterIndex + 1.0 ) ) * cos( *( individual + parameterIndex ) / sqrt( parameterIndex + 1.0 ) ) ); 
 }
 return additivePartOfGriewankFunctionValue - multiplicativePartOfGriewankFunctionValue + 1.0;
}
double calculatePenalizedFunction( double* individual ){
 double firstDouble_Variable = 0.0;
 double secondDouble_Variable = 0.0;
 for( int parameterIndex = 0; parameterIndex < NUMBEROFPARAMETERS - 1; parameterIndex ++ ){
  firstDouble_Variable = firstDouble_Variable + pow( ( 1.0 / 4.0 ) * ( *( individual + parameterIndex ) + 1.0 ), 2.0 ) * ( 1.0 + 10.0 * pow( sin( PI * ( 1.0 + ( 1.0 / 4.0 ) * ( *( individual + parameterIndex + 1 ) + 1.0 ) ) ), 2.0 ) );
 }
 firstDouble_Variable = firstDouble_Variable + 10.0 * pow( sin( PI * ( 1.0 + ( 1.0 / 4.0 ) * ( ( *individual ) + 1.0 ) ) ) , 2.0 ) + pow( ( ( 1.0 / 4.0 ) * *( individual + NUMBEROFPARAMETERS - 1 ) ), 2.0 ); 
 for( int parameterIndex = 0; parameterIndex < NUMBEROFPARAMETERS; parameterIndex ++ ){
  if( *( individual + parameterIndex ) > 10 ){
   secondDouble_Variable = secondDouble_Variable + 100.0 * pow( *( individual + parameterIndex ) - 10.0, 4.0 );
  }
  else if( *( individual + parameterIndex ) < -10 ){
   secondDouble_Variable = secondDouble_Variable + 100.0 * pow( -( *( individual + parameterIndex ) + 10.0 ), 4.0 );
  } 
 }
 return ( PI / ( 1.0 * NUMBEROFPARAMETERS ) ) * firstDouble_Variable + secondDouble_Variable;
}
double calculatePenalized1Function( double* individual ){
 double firstDouble_Variable = 0.0;
 double secondDouble_Variable = 0.0;
 for( int parameterIndex = 0; parameterIndex < NUMBEROFPARAMETERS - 1; parameterIndex ++ ){
  firstDouble_Variable = firstDouble_Variable + pow( ( 1.0 / 4.0 ) * ( *( individual + parameterIndex ) + 1.0 ), 2.0 ) * ( 1.0 + 10.0 * pow( sin( PI * ( 1.0 + ( 1.0 / 4.0 ) * ( *( individual + parameterIndex + 1 ) + 1.0 ) ) ), 2.0 ) );
 }
 firstDouble_Variable = firstDouble_Variable + 10.0 * pow( sin( PI * ( 1.0 + ( 1.0 / 4.0 ) * ( ( *individual ) + 1.0 ) ) ) , 2.0 ) + pow( ( ( 1.0 / 4.0 ) * *( individual + NUMBEROFPARAMETERS - 1 ) ), 2.0 ); 
 for( int parameterIndex = 0; parameterIndex < NUMBEROFPARAMETERS; parameterIndex ++ ){
  if( *( individual + parameterIndex ) > 10 ){
   secondDouble_Variable = secondDouble_Variable + 100.0 * pow( *( individual + parameterIndex ) - 10.0, 4.0 );
  }
  else if( *( individual + parameterIndex ) < -10 ){
   secondDouble_Variable = secondDouble_Variable + 100.0 * pow( -( *( individual + parameterIndex ) + 10.0 ), 4.0 );
  } 
 }
 return ( 0.1 ) * firstDouble_Variable + secondDouble_Variable;
}
double calculateSumSquareFunction( double* individual ){
 double sphereFunctionValue = 0.0;
 for( int parameterIndex = 0; parameterIndex < NUMBEROFPARAMETERS; parameterIndex ++ ){
  sphereFunctionValue = sphereFunctionValue + ( parameterIndex + 1 ) * *( individual + parameterIndex ) * *( individual + parameterIndex ); 
 }
 return sphereFunctionValue;
}
double calculateRastriginFunction( double* individual ){
 double rastriginFunctionValue = 0.0;
 for( int parameterIndex = 0; parameterIndex < NUMBEROFPARAMETERS; parameterIndex ++ ){
  rastriginFunctionValue = rastriginFunctionValue + ( *( individual + parameterIndex ) * *( individual + parameterIndex ) - 10.0 * cos( 2.0 * PI * *( individual + parameterIndex ) ) );
 }
 return 10.0 * NUMBEROFPARAMETERS + rastriginFunctionValue;
}
double calculateDixonPriceFunction( double* individual ){
 double firstDouble_Variable = 0.0;
 for( int parameterIndex = 1; parameterIndex < NUMBEROFPARAMETERS; parameterIndex ++ ){
  firstDouble_Variable = firstDouble_Variable + ( ( parameterIndex + 1.0 ) * 1.0 ) * pow( ( 2.0 * pow( *( individual + parameterIndex ) , 2 ) - *( individual + parameterIndex - 1 ) ) , 2 );
 }
 return pow( ( ( *individual ) - 1.0 ) , 2 ) + firstDouble_Variable;
}
double calculateSchwefel12Function( double* individual ){
 double firstDouble_Variable = 0.0;
 double secondDouble_Variable = 0.0;
 for( int parameterIndex = 0; parameterIndex < NUMBEROFPARAMETERS; parameterIndex ++ ){
  secondDouble_Variable = 0.0;
  for( int firstParameterIndex = 0; firstParameterIndex <= parameterIndex; firstParameterIndex ++ ){
   secondDouble_Variable = secondDouble_Variable + *( individual + firstParameterIndex );
  }
  firstDouble_Variable = firstDouble_Variable + pow( secondDouble_Variable, 2.0 );
 }
 return firstDouble_Variable;
}
double calculateSchwefel221Function( double* individual ){
 double firstDouble_Variable = abs( *( individual ) );
 for( int parameterIndex = 1; parameterIndex < NUMBEROFPARAMETERS; parameterIndex ++ ){
  if( firstDouble_Variable < fabs( *( individual + parameterIndex ) ) ){
   firstDouble_Variable = fabs( *( individual + parameterIndex ) );
  }
 }
 return firstDouble_Variable;
}
double calculateSchwefel222Function( double* individual ){
 double firstDouble_Variable = 0.0;
 double secondDouble_Variable = 1.0;
 for( int parameterIndex = 0; parameterIndex < NUMBEROFPARAMETERS; parameterIndex ++ ){
  firstDouble_Variable = firstDouble_Variable + fabs( *( individual + parameterIndex ) ); 
  secondDouble_Variable = secondDouble_Variable * fabs( *( individual + parameterIndex ) );
 }
 return firstDouble_Variable + secondDouble_Variable;
}
double calculateRosenbrockValleyFunction( double* individual ){
 double rosenbrockValleyFunctionValue = 0.0;
 for( int parameterIndex = 0; parameterIndex < NUMBEROFPARAMETERS - 1; parameterIndex ++ ){
  rosenbrockValleyFunctionValue = rosenbrockValleyFunctionValue + 100.0 * ( *( individual + ( parameterIndex + 1 ) ) - *( individual + parameterIndex ) * *( individual + parameterIndex ) ) * ( *( individual + ( parameterIndex + 1 ) ) - *( individual + parameterIndex ) * *( individual + parameterIndex ) ) + ( *( individual + parameterIndex ) - 1.0 ) * ( *( individual + parameterIndex ) - 1.0 );
 }
 return rosenbrockValleyFunctionValue;
}

void immuneSystemPhase( long threadRank ){
 double* individualsInThread = individuals[ threadRank ];	
 long* evaluationIndexInThread = evaluationIndexes[ threadRank ];
 double* objectiveFunctionValuesInThread = objectiveFunctionValues[ threadRank ];
 double* bestObjectiveFunctionValueInThread = bestObjectiveFunctionValues[ threadRank ];
 double* bestObjectiveFunctionValuesOfEvaluationsInThread = bestObjectiveFunctionValuesOfEvaluations[ threadRank ];
 
 double* individual = NULL;
 double* randomIndividual = NULL;
 
 int randomIndividualIndex = 0;
 int changedParameterIndex = 0;
 double individualParameter = 0.0f;
 double randomIndividualParameter = 0.0f;
 double candidateIndividualParameter = 0.0f;
 double objectiveFunctionValueOfCandidateIndividual = 0.0f;
 
 for( int individualIndex = 0; individualIndex < NUMBEROFINDIVIDUALSINTHREAD; individualIndex ++ ){
  if( *( evaluationIndexInThread ) < MAXIMUMFITNESSEVALUATIONS ){
   //Increase evaluation numbers
   *( evaluationIndexInThread ) = *( evaluationIndexInThread ) + 1;
   //Increase evaluation numbers
  	
   //Determine changed parameter and random individual index
   changedParameterIndex = getChangedParameterIndex();
   randomIndividualIndex = getRandomIndividualIndex( individualIndex );
   //Determine changed parameter and random individual index

   //Set pointers to related individuals
   individual = ( double* )( ( char* )individualsInThread + individualIndex * NUMBEROFPARAMETERS * sizeof( double ) );
   randomIndividual = ( double* )( ( char* )individualsInThread + randomIndividualIndex * NUMBEROFPARAMETERS * sizeof( double ) );
   //Set pointers to related individuals

   //Change individual with the random individual
   individualParameter = *( individual + changedParameterIndex );
   randomIndividualParameter = *( randomIndividual + changedParameterIndex );
   candidateIndividualParameter = individualParameter + ( ( ( ( 1.0 * rand() ) / ( RAND_MAX + 1.0 ) ) - 0.5 ) * ( 2.0 ) ) * ( individualParameter - randomIndividualParameter );
   if (candidateIndividualParameter > upperBounds[changedParameterIndex]) {
       candidateIndividualParameter = upperBounds[changedParameterIndex];
   }

   else if (candidateIndividualParameter < lowerBounds[changedParameterIndex]) {
       candidateIndividualParameter = lowerBounds[changedParameterIndex];
   }

   *( individual + changedParameterIndex ) = candidateIndividualParameter;
   //Change individual with the random individual

   //Apply greedy selection between individuals
   calculateObjectiveFunctionValue( individual, &objectiveFunctionValueOfCandidateIndividual, OBJECTIVEFUNCTION );
   if( objectiveFunctionValueOfCandidateIndividual > *( objectiveFunctionValuesInThread + individualIndex ) ){
    *( individual + changedParameterIndex ) = individualParameter;
   }
   else{
    *( objectiveFunctionValuesInThread + individualIndex ) = objectiveFunctionValueOfCandidateIndividual;
   }
   //Apply greedy selection between individuals

   //Update global best individual
   if( *( bestObjectiveFunctionValueInThread ) > *( objectiveFunctionValuesInThread + individualIndex ) ){
    *( bestObjectiveFunctionValueInThread ) = *( objectiveFunctionValuesInThread + individualIndex );
   }
   *( bestObjectiveFunctionValuesOfEvaluationsInThread + *( evaluationIndexInThread ) - 1 ) = *( bestObjectiveFunctionValueInThread );
   //Update global best individual
  }
 }  
}
void convalescentTreatmentPhase( long threadRank ){
 double* individualsInThread = individuals[ threadRank ];	
 int* indexesOfDonorsInThread = indexesOfDonors[ threadRank ];
 long* evaluationIndexInThread = evaluationIndexes[ threadRank ];
 int* indexesOfReceiversInThread = indexesOfReceivers[ threadRank ];
 double* objectiveFunctionValuesInThread = objectiveFunctionValues[ threadRank ];
 double* bestObjectiveFunctionValueInThread = bestObjectiveFunctionValues[ threadRank ];
 double* bestObjectiveFunctionValuesOfEvaluationsInThread = bestObjectiveFunctionValuesOfEvaluations[ threadRank ];

 double* donorIndividual = NULL;
 double* receiverIndividual = NULL;
 double* candidateIndividual = ( double* )malloc( NUMBEROFPARAMETERS * sizeof( double ) );
 
 int donorIndividualIndex = 0;
 int receiverIndividualIndex = 0;
 double donorIndividualParameter = 0.0f;
 double receiverIndividualParameter = 0.0f;
 double candidateIndividualParameter = 0.0f; 
 double objectiveFunctionValueOfCandidateIndividual = 0.0f;
 int* treatmentControls = ( int* )malloc( NUMBEROFRECEIVERS * sizeof( int ) );
 
 //Get the indexes of the donor and receiver individuals
 getDonorAndReceiverIndexes( threadRank );
 //Get the indexes of the donor and receiver individuals

 //Assign initial values to the treatmentControls
 for( int receiverIndex = 0; receiverIndex < NUMBEROFRECEIVERS; receiverIndex ++ ){
  *( treatmentControls + receiverIndex ) = 1;
 }
 //Assign initial values to the treatmentControls
 
 for( int index = 0; index < NUMBEROFRECEIVERS; index ++ ){
  //Set pointers to related individuals
  receiverIndividualIndex = *( indexesOfReceiversInThread + index );
  donorIndividualIndex = *( indexesOfDonorsInThread + ( int )( ( ( 1.0 * rand() ) / ( RAND_MAX + 1.0 ) ) * NUMBEROFDONORS ) );
  donorIndividual = ( double* )( ( char* )individualsInThread + donorIndividualIndex * NUMBEROFPARAMETERS * sizeof( double ) );
  receiverIndividual = ( double* )( ( char* )individualsInThread + receiverIndividualIndex * NUMBEROFPARAMETERS * sizeof( double ) );
  //Set pointers to related individuals

  while( *( treatmentControls + index ) == 1 ){
   if( *( evaluationIndexInThread ) < MAXIMUMFITNESSEVALUATIONS ){
    //Increase evaluation numbers
    *( evaluationIndexInThread ) = *( evaluationIndexInThread ) + 1;
    //Increase evaluation numbers
    
	//Change individual
    for( int parameterIndex = 0; parameterIndex < NUMBEROFPARAMETERS; parameterIndex ++ ){
     donorIndividualParameter = *( donorIndividual + parameterIndex );
     receiverIndividualParameter = *( receiverIndividual + parameterIndex );
     candidateIndividualParameter = receiverIndividualParameter + ( ( ( ( 1.0 * rand() ) / ( RAND_MAX + 1.0 ) ) - 0.5 ) * ( 2.0 ) ) * ( receiverIndividualParameter - donorIndividualParameter );
     if (candidateIndividualParameter > upperBounds[parameterIndex]) {
         candidateIndividualParameter = upperBounds[parameterIndex];
     }
     if (candidateIndividualParameter < lowerBounds[parameterIndex]) {
         candidateIndividualParameter = lowerBounds[parameterIndex];
     }


                    void convalescentTreatmentPhase( long threadRank ){
                     double* individualsInThread = individuals[ threadRank ];	
                     int* indexesOfDonorsInThread = indexesOfDonors[ threadRank ];
                     long* evaluationIndexInThread = evaluationIndexes[ threadRank ];
                     int* indexesOfReceiversInThread = indexesOfReceivers[ threadRank ];
                     double* objectiveFunctionValuesInThread = objectiveFunctionValues[ threadRank ];
                     double* bestObjectiveFunctionValueInThread = bestObjectiveFunctionValues[ threadRank ];
                     double* bestObjectiveFunctionValuesOfEvaluationsInThread = bestObjectiveFunctionValuesOfEvaluations[ threadRank ];

                     double* donorIndividual = NULL;
                     double* receiverIndividual = NULL;
                     double* candidateIndividual = ( double* )malloc( NUMBEROFPARAMETERS * sizeof( double ) );
                     
                     int donorIndividualIndex = 0;
                     int receiverIndividualIndex = 0;
                     double donorIndividualParameter = 0.0f;
                     double receiverIndividualParameter = 0.0f;
                     double candidateIndividualParameter = 0.0f; 
                     double objectiveFunctionValueOfCandidateIndividual = 0.0f;
                     int* treatmentControls = ( int* )malloc( NUMBEROFRECEIVERS * sizeof( int ) );
                     
                     //Get the indexes of the donor and receiver individuals
                     getDonorAndReceiverIndexes( threadRank );
                     //Get the indexes of the donor and receiver individuals

                     //Assign initial values to the treatmentControls
                     for( int receiverIndex = 0; receiverIndex < NUMBEROFRECEIVERS; receiverIndex ++ ){
                            void convalescentTreatmentPhase( long threadRank ){
                             double* individualsInThread = individuals[ threadRank ];	
                             int* indexesOfDonorsInThread = indexesOfDonors[ threadRank ];
                             long* evaluationIndexInThread = evaluationIndexes[ threadRank ];
                             int* indexesOfReceiversInThread = indexesOfReceivers[ threadRank ];
                             double* objectiveFunctionValuesInThread = objectiveFunctionValues[ threadRank ];
                             double* bestObjectiveFunctionValueInThread = bestObjectiveFunctionValues[ threadRank ];
                             double* bestObjectiveFunctionValuesOfEvaluationsInThread = bestObjectiveFunctionValuesOfEvaluations[ threadRank ];

                             double* donorIndividual = NULL;
                             double* receiverIndividual = NULL;
                             double* candidateIndividual = ( double* )malloc( NUMBEROFPARAMETERS * sizeof( double ) );
                             
                             int donorIndividualIndex = 0;
                             int receiverIndividualIndex = 0;
                             double donorIndividualParameter = 0.0f;
                             double receiverIndividualParameter = 0.0f;
                             double candidateIndividualParameter = 0.0f; 
                             double objectiveFunctionValueOfCandidateIndividual = 0.0f;
                             int* treatmentControls = ( int* )malloc( NUMBEROFRECEIVERS * sizeof( int ) );
                             
                             //Get the indexes of the donor and receiver individuals
                             getDonorAndReceiverIndexes( threadRank );
                             //Get the indexes of the donor and receiver individuals

                             //Assign initial values to the treatmentControls
                             for( int receiverIndex = 0; receiverIndex < NUMBEROFRECEIVERS; receiverIndex ++ ){
                              *( treatmentControls + receiverIndex ) = 1;
                             }
                             //Assign initial values to the treatmentControls
                             
                             for( int index = 0; index < NUMBEROFRECEIVERS; index ++ ){
                              //Set pointers to related individuals
                              receiverIndividualIndex = *( indexesOfReceiversInThread + index );
                              donorIndividualIndex = *( indexesOfDonorsInThread + ( int )( ( ( 1.0 * rand() ) / ( RAND_MAX + 1.0 ) ) * NUMBEROFDONORS ) );
                              donorIndividual = ( double* )( ( char* )individualsInThread + donorIndividualIndex * NUMBEROFPARAMETERS * sizeof( double ) );
                              receiverIndividual = ( double* )( ( char* )individualsInThread + receiverIndividualIndex * NUMBEROFPARAMETERS * sizeof( double ) );
                              //Set pointers to related individuals

                              while( *( treatmentControls + index ) == 1 ){
                               if( *( evaluationIndexInThread ) < MAXIMUMFITNESSEVALUATIONS ){
                                //Increase evaluation numbers
                                *( evaluationIndexInThread ) = *( evaluationIndexInThread ) + 1;
                                //Increase evaluation numbers
                                
																													//Change individual
                                for( int parameterIndex = 0; parameterIndex < NUMBEROFPARAMETERS; parameterIndex ++ ){
                                 donorIndividualParameter = *( donorIndividual + parameterIndex );
                                 receiverIndividualParameter = *( receiverIndividual + parameterIndex );
                                 candidateIndividualParameter = receiverIndividualParameter + ( ( ( ( 1.0 * rand() ) / ( RAND_MAX + 1.0 ) ) - 0.5 ) * ( 2.0 ) ) * ( receiverIndividualParameter - donorIndividualParameter );
                                 if (candidateIndividualParameter > upperBounds[parameterIndex]) {
                                     candidateIndividualParameter = upperBounds[parameterIndex];
                                 }
                                 if (candidateIndividualParameter < lowerBounds[parameterIndex]) {
                                     candidateIndividualParameter = lowerBounds[parameterIndex];
                                 }

                                 *( candidateIndividual + parameterIndex ) = candidateIndividualParameter;
																													}
                                //Change individual

                                //Apply greedy selection between individuals
                                calculateObjectiveFunctionValue( candidateIndividual, &objectiveFunctionValueOfCandidateIndividual, OBJECTIVEFUNCTION );
                                if( objectiveFunctionValueOfCandidateIndividual < *( objectiveFunctionValuesInThread + donorIndividualIndex ) && objectiveFunctionValueOfCandidateIndividual < *( objectiveFunctionValuesInThread + receiverIndividualIndex ) ){
                                 for( int parameterIndex = 0; parameterIndex < NUMBEROFPARAMETERS; parameterIndex ++ ){
                                  *( receiverIndividual + parameterIndex ) = *( candidateIndividual + parameterIndex );	
                              }
                                 *( objectiveFunctionValuesInThread + receiverIndividualIndex ) = objectiveFunctionValueOfCandidateIndividual;
                                }
                                else{
                                 if( *( objectiveFunctionValuesInThread + donorIndividualIndex ) < *( objectiveFunctionValuesInThread + receiverIndividualIndex ) ){
                                  for( int parameterIndex = 0; parameterIndex < NUMBEROFPARAMETERS; parameterIndex ++ ){
                                   *( receiverIndividual + parameterIndex ) = *( donorIndividual + parameterIndex );
                                  }
                                  *( objectiveFunctionValuesInThread + receiverIndividualIndex ) = *( objectiveFunctionValuesInThread + donorIndividualIndex );
                                 }
                              *( treatmentControls + index ) = 0;
																													}
                                //Apply greedy selection between individuals
                                
                                //Update global best individual
                                if( *( bestObjectiveFunctionValueInThread ) > objectiveFunctionValueOfCandidateIndividual ){
                                 *( bestObjectiveFunctionValueInThread ) = objectiveFunctionValueOfCandidateIndividual;
                                }
                                *( bestObjectiveFunctionValuesOfEvaluationsInThread + *( evaluationIndexInThread ) - 1 ) = *( bestObjectiveFunctionValueInThread );
                                //Update global best individual
                               }
                               else{
                                *( treatmentControls + index ) = 0;
                               }
                              }
                             }

                             //Modifying donor individuals
                             for( int donorIndex = 0; donorIndex < NUMBEROFDONORS; donorIndex ++ ){
                              double randomValue = ( 1.0 * rand() ) / ( RAND_MAX + 1.0 );	
                              if( *( evaluationIndexInThread ) < MAXIMUMFITNESSEVALUATIONS ){
                               //Increase evaluation numbers
                               *( evaluationIndexInThread ) = *( evaluationIndexInThread ) + 1;
                               //Increase evaluation numbers
                               
                               //Set pointers to related individual
                               donorIndividual = ( double* )( ( char* )individualsInThread + *( indexesOfDonorsInThread + donorIndex ) * NUMBEROFPARAMETERS * sizeof( double ) );
                               //Set pointers to related individual
                               if( randomValue < 1.0 * *( evaluationIndexInThread ) / MAXIMUMFITNESSEVALUATIONS ){
                                for( int parameterIndex = 0; parameterIndex < NUMBEROFPARAMETERS; parameterIndex ++ ){
                                 *( donorIndividual + parameterIndex ) = *( donorIndividual + parameterIndex ) + ( ( ( ( rand() ) / ( RAND_MAX + 1.0 ) ) - 0.5 ) * ( 2.0 ) ) * ( *( donorIndividual + parameterIndex ) );
                                 if( *( donorIndividual + parameterIndex ) > upperBounds[parameterIndex] ){
                                  *( donorIndividual + parameterIndex ) = upperBounds[parameterIndex];
                                 }
                                 else if( *( donorIndividual + parameterIndex ) < lowerBounds[parameterIndex] ){
                                  *( donorIndividual + parameterIndex ) = lowerBounds[parameterIndex];
                                 }
                                }
                               }
                               else{
                                for( int parameterIndex = 0; parameterIndex < NUMBEROFPARAMETERS; parameterIndex ++ ){
                                 *( donorIndividual + parameterIndex ) = lowerBounds[parameterIndex] + ( ( ( ( rand() ) / ( RAND_MAX + 1.0 ) ) - 0.5 ) * ( 2.0 ) ) * ( upperBounds[parameterIndex] - lowerBounds[parameterIndex] );
                                 if( *( donorIndividual + parameterIndex ) > upperBounds[parameterIndex] ){
                                  *( donorIndividual + parameterIndex ) = upperBounds[parameterIndex];
                                 }
                                 else if( *( donorIndividual + parameterIndex ) < lowerBounds[parameterIndex] ){
                                  *( donorIndividual + parameterIndex ) = lowerBounds[parameterIndex];
                                 }
                                }
                               }
                               calculateObjectiveFunctionValue( donorIndividual, ( objectiveFunctionValuesInThread + *( indexesOfDonorsInThread + donorIndex ) ), OBJECTIVEFUNCTION );
                               //Update global best individual
                               if( *( bestObjectiveFunctionValueInThread ) > *( objectiveFunctionValuesInThread + *( indexesOfDonorsInThread + donorIndex ) ) ){
                                *( bestObjectiveFunctionValueInThread ) = *( objectiveFunctionValuesInThread + *( indexesOfDonorsInThread + donorIndex ) );
                               }
                               *( bestObjectiveFunctionValuesOfEvaluationsInThread + *( evaluationIndexInThread ) - 1 ) = *( bestObjectiveFunctionValueInThread );  
                               //Update global best individual
                              }
                             }
                             //Modifying donor individuals

                             free( treatmentControls );
                             free( candidateIndividual );
                            }
                      *( treatmentControls + receiverIndex ) = 1;
                     }
                     //Assign initial values to the treatmentControls
                     
                     for( int index = 0; index < NUMBEROFRECEIVERS; index ++ ){
                      //Set pointers to related individuals
                      receiverIndividualIndex = *( indexesOfReceiversInThread + index );
                      donorIndividualIndex = *( indexesOfDonorsInThread + ( int )( ( ( 1.0 * rand() ) / ( RAND_MAX + 1.0 ) ) * NUMBEROFDONORS ) );
                      donorIndividual = ( double* )( ( char* )individualsInThread + donorIndividualIndex * NUMBEROFPARAMETERS * sizeof( double ) );
                      receiverIndividual = ( double* )( ( char* )individualsInThread + receiverIndividualIndex * NUMBEROFPARAMETERS * sizeof( double ) );
                      //Set pointers to related individuals

                      while( *( treatmentControls + index ) == 1 ){
                       if( *( evaluationIndexInThread ) < MAXIMUMFITNESSEVALUATIONS ){
                        //Increase evaluation numbers
                        *( evaluationIndexInThread ) = *( evaluationIndexInThread ) + 1;
                        //Increase evaluation numbers
                        
																					//Change individual
                        for( int parameterIndex = 0; parameterIndex < NUMBEROFPARAMETERS; parameterIndex ++ ){
                         donorIndividualParameter = *( donorIndividual + parameterIndex );
                         receiverIndividualParameter = *( receiverIndividual + parameterIndex );
                         candidateIndividualParameter = receiverIndividualParameter + ( ( ( ( 1.0 * rand() ) / ( RAND_MAX + 1.0 ) ) - 0.5 ) * ( 2.0 ) ) * ( receiverIndividualParameter - donorIndividualParameter );
                         if (candidateIndividualParameter > upperBounds[parameterIndex]) {
                             candidateIndividualParameter = upperBounds[parameterIndex];
                         }
                         if (candidateIndividualParameter < lowerBounds[parameterIndex]) {
                             candidateIndividualParameter = lowerBounds[parameterIndex];
                         }


                         *( candidateIndividual + parameterIndex ) = candidateIndividualParameter;
																					}
                        //Change individual

                        //Apply greedy selection between individuals
                        calculateObjectiveFunctionValue( candidateIndividual, &objectiveFunctionValueOfCandidateIndividual, OBJECTIVEFUNCTION );
                        if( objectiveFunctionValueOfCandidateIndividual < *( objectiveFunctionValuesInThread + donorIndividualIndex ) && objectiveFunctionValueOfCandidateIndividual < *( objectiveFunctionValuesInThread + receiverIndividualIndex ) ){
                         for( int parameterIndex = 0; parameterIndex < NUMBEROFPARAMETERS; parameterIndex ++ ){
                          *( receiverIndividual + parameterIndex ) = *( candidateIndividual + parameterIndex );	
                      }
                         *( objectiveFunctionValuesInThread + receiverIndividualIndex ) = objectiveFunctionValueOfCandidateIndividual;
                        }
                        else{
                         if( *( objectiveFunctionValuesInThread + donorIndividualIndex ) < *( objectiveFunctionValuesInThread + receiverIndividualIndex ) ){
                          for( int parameterIndex = 0; parameterIndex < NUMBEROFPARAMETERS; parameterIndex ++ ){
                           *( receiverIndividual + parameterIndex ) = *( donorIndividual + parameterIndex );
                          }
                          *( objectiveFunctionValuesInThread + receiverIndividualIndex ) = *( objectiveFunctionValuesInThread + donorIndividualIndex );
                         }
                      *( treatmentControls + index ) = 0;
																					}
                        //Apply greedy selection between individuals
                        
                        //Update global best individual
                        if( *( bestObjectiveFunctionValueInThread ) > objectiveFunctionValueOfCandidateIndividual ){
                         *( bestObjectiveFunctionValueInThread ) = objectiveFunctionValueOfCandidateIndividual;
                        }
                        *( bestObjectiveFunctionValuesOfEvaluationsInThread + *( evaluationIndexInThread ) - 1 ) = *( bestObjectiveFunctionValueInThread );
                        //Update global best individual
                       }
                       else{
                        *( treatmentControls + index ) = 0;
                       }
                      }
                     }

                     //Modifying donor individuals
                     for( int donorIndex = 0; donorIndex < NUMBEROFDONORS; donorIndex ++ ){
                      double randomValue = ( 1.0 * rand() ) / ( RAND_MAX + 1.0 );	
                      if( *( evaluationIndexInThread ) < MAXIMUMFITNESSEVALUATIONS ){
                       //Increase evaluation numbers
                       *( evaluationIndexInThread ) = *( evaluationIndexInThread ) + 1;
                       //Increase evaluation numbers
                       
                       //Set pointers to related individual
                       donorIndividual = ( double* )( ( char* )individualsInThread + *( indexesOfDonorsInThread + donorIndex ) * NUMBEROFPARAMETERS * sizeof( double ) );
                       //Set pointers to related individual
                       if( randomValue < 1.0 * *( evaluationIndexInThread ) / MAXIMUMFITNESSEVALUATIONS ){
                        for( int parameterIndex = 0; parameterIndex < NUMBEROFPARAMETERS; parameterIndex ++ ){
                         *( donorIndividual + parameterIndex ) = *( donorIndividual + parameterIndex ) + ( ( ( ( rand() ) / ( RAND_MAX + 1.0 ) ) - 0.5 ) * ( 2.0 ) ) * ( *( donorIndividual + parameterIndex ) );
                         if( *( donorIndividual + parameterIndex ) > upperBounds[parameterIndex] ){
                          *( donorIndividual + parameterIndex ) = upperBounds[parameterIndex];
                         }
                         else if( *( donorIndividual + parameterIndex ) < lowerBounds[parameterIndex] ){
                          *( donorIndividual + parameterIndex ) = lowerBounds[parameterIndex];
                         }
                        }
                       }
                       else{
                        for( int parameterIndex = 0; parameterIndex < NUMBEROFPARAMETERS; parameterIndex ++ ){
                         *( donorIndividual + parameterIndex ) = lowerBounds[parameterIndex] + ( ( ( ( rand() ) / ( RAND_MAX + 1.0 ) ) - 0.5 ) * ( 2.0 ) ) * ( upperBounds[parameterIndex] - lowerBounds[parameterIndex] );
                         if( *( donorIndividual + parameterIndex ) > upperBounds[parameterIndex] ){
                          *( donorIndividual + parameterIndex ) = upperBounds[parameterIndex];
                         }
                         else if( *( donorIndividual + parameterIndex ) < lowerBounds[parameterIndex] ){
                          *( donorIndividual + parameterIndex ) = lowerBounds[parameterIndex];
                         }
                        }
                       }
                       calculateObjectiveFunctionValue( donorIndividual, ( objectiveFunctionValuesInThread + *( indexesOfDonorsInThread + donorIndex ) ), OBJECTIVEFUNCTION );
                       //Update global best individual
                       if( *( bestObjectiveFunctionValueInThread ) > *( objectiveFunctionValuesInThread + *( indexesOfDonorsInThread + donorIndex ) ) ){
                        *( bestObjectiveFunctionValueInThread ) = *( objectiveFunctionValuesInThread + *( indexesOfDonorsInThread + donorIndex ) );
                       }
                       *( bestObjectiveFunctionValuesOfEvaluationsInThread + *( evaluationIndexInThread ) - 1 ) = *( bestObjectiveFunctionValueInThread );  
                       //Update global best individual
                      }
                     }
                     //Modifying donor individuals

                     free( treatmentControls );
                     free( candidateIndividual );
                    }
     *( candidateIndividual + parameterIndex ) = candidateIndividualParameter;
	}
    //Change individual

    //Apply greedy selection between individuals
    calculateObjectiveFunctionValue( candidateIndividual, &objectiveFunctionValueOfCandidateIndividual, OBJECTIVEFUNCTION );
    if( objectiveFunctionValueOfCandidateIndividual < *( objectiveFunctionValuesInThread + donorIndividualIndex ) && objectiveFunctionValueOfCandidateIndividual < *( objectiveFunctionValuesInThread + receiverIndividualIndex ) ){
     for( int parameterIndex = 0; parameterIndex < NUMBEROFPARAMETERS; parameterIndex ++ ){
      *( receiverIndividual + parameterIndex ) = *( candidateIndividual + parameterIndex );	
	 }
     *( objectiveFunctionValuesInThread + receiverIndividualIndex ) = objectiveFunctionValueOfCandidateIndividual;
    }
    else{
     if( *( objectiveFunctionValuesInThread + donorIndividualIndex ) < *( objectiveFunctionValuesInThread + receiverIndividualIndex ) ){
      for( int parameterIndex = 0; parameterIndex < NUMBEROFPARAMETERS; parameterIndex ++ ){
       *( receiverIndividual + parameterIndex ) = *( donorIndividual + parameterIndex );
      }
      *( objectiveFunctionValuesInThread + receiverIndividualIndex ) = *( objectiveFunctionValuesInThread + donorIndividualIndex );
     }
	 *( treatmentControls + index ) = 0;
	}
    //Apply greedy selection between individuals
    
    //Update global best individual
    if( *( bestObjectiveFunctionValueInThread ) > objectiveFunctionValueOfCandidateIndividual ){
     *( bestObjectiveFunctionValueInThread ) = objectiveFunctionValueOfCandidateIndividual;
    }
    *( bestObjectiveFunctionValuesOfEvaluationsInThread + *( evaluationIndexInThread ) - 1 ) = *( bestObjectiveFunctionValueInThread );
    //Update global best individual
   }
   else{
    *( treatmentControls + index ) = 0;
   }
  }
 }

 //Modifying donor individuals
 for( int donorIndex = 0; donorIndex < NUMBEROFDONORS; donorIndex ++ ){
  double randomValue = ( 1.0 * rand() ) / ( RAND_MAX + 1.0 );	
  if( *( evaluationIndexInThread ) < MAXIMUMFITNESSEVALUATIONS ){
   //Increase evaluation numbers
   *( evaluationIndexInThread ) = *( evaluationIndexInThread ) + 1;
   //Increase evaluation numbers
   
   //Set pointers to related individual
   donorIndividual = ( double* )( ( char* )individualsInThread + *( indexesOfDonorsInThread + donorIndex ) * NUMBEROFPARAMETERS * sizeof( double ) );
   //Set pointers to related individual
   if( randomValue < 1.0 * *( evaluationIndexInThread ) / MAXIMUMFITNESSEVALUATIONS ){
    for( int parameterIndex = 0; parameterIndex < NUMBEROFPARAMETERS; parameterIndex ++ ){
     *( donorIndividual + parameterIndex ) = *( donorIndividual + parameterIndex ) + ( ( ( ( rand() ) / ( RAND_MAX + 1.0 ) ) - 0.5 ) * ( 2.0 ) ) * ( *( donorIndividual + parameterIndex ) );
     if (*(donorIndividual + parameterIndex) > upperBounds[parameterIndex]) {
         *(donorIndividual + parameterIndex) = upperBounds[parameterIndex];
     }
     else if (*(donorIndividual + parameterIndex) < lowerBounds[parameterIndex]) {
         *(donorIndividual + parameterIndex) = lowerBounds[parameterIndex];
     }

    }
   }
   else{
    for( int parameterIndex = 0; parameterIndex < NUMBEROFPARAMETERS; parameterIndex ++ ){
        *(donorIndividual + parameterIndex) = lowerBounds[parameterIndex] +
            ((((rand()) / (RAND_MAX + 1.0)) - 0.5) * (2.0)) *
            (upperBounds[parameterIndex] - lowerBounds[parameterIndex]);

        if (*(donorIndividual + parameterIndex) > upperBounds[parameterIndex]) {
            *(donorIndividual + parameterIndex) = upperBounds[parameterIndex];
        }
        else if (*(donorIndividual + parameterIndex) < lowerBounds[parameterIndex]) {
            *(donorIndividual + parameterIndex) = lowerBounds[parameterIndex];
        }

    }
   }
   calculateObjectiveFunctionValue( donorIndividual, ( objectiveFunctionValuesInThread + *( indexesOfDonorsInThread + donorIndex ) ), OBJECTIVEFUNCTION );
   //Update global best individual
   if( *( bestObjectiveFunctionValueInThread ) > *( objectiveFunctionValuesInThread + *( indexesOfDonorsInThread + donorIndex ) ) ){
    *( bestObjectiveFunctionValueInThread ) = *( objectiveFunctionValuesInThread + *( indexesOfDonorsInThread + donorIndex ) );
   }
   *( bestObjectiveFunctionValuesOfEvaluationsInThread + *( evaluationIndexInThread ) - 1 ) = *( bestObjectiveFunctionValueInThread );  
   //Update global best individual
  }
 }
 //Modifying donor individuals

 free( treatmentControls );
 free( candidateIndividual );
}

void getDonorAndReceiverIndexes( long threadRank ){
 int* indexesOfDonorsInThread = indexesOfDonors[ threadRank ];
 int* indexesOfReceiversInThread = indexesOfReceivers[ threadRank ];
 double* objectiveFunctionValuesInThread = objectiveFunctionValues[ threadRank ];
 int* temporaryIndexesOfIndividualsInThread = ( int* )malloc( NUMBEROFINDIVIDUALSINTHREAD * sizeof( int ) );
 double* temporaryObjectiveFunctionValuesInThread = ( double* )malloc( NUMBEROFINDIVIDUALSINTHREAD * sizeof( double ) );
 
 //Copy the objective function values 
 for( int individualIndex = 0; individualIndex < NUMBEROFINDIVIDUALSINTHREAD; individualIndex ++ ){
  *( temporaryIndexesOfIndividualsInThread + individualIndex ) = individualIndex;
  *( temporaryObjectiveFunctionValuesInThread + individualIndex ) = *( objectiveFunctionValuesInThread + individualIndex );
 }
 //Copy the objective function values
 
 //Sort the individuals using objective function values
 int temporaryIndexOfIndividualInThread = 0;
 double temporaryObjectiveFunctionValueInThread = 0.0f;
 for( int individualIndex = 0; individualIndex < NUMBEROFINDIVIDUALSINTHREAD - 1; individualIndex ++ ){
  for( int temporaryIndividualIndex = individualIndex + 1; temporaryIndividualIndex < NUMBEROFINDIVIDUALSINTHREAD; temporaryIndividualIndex ++ ){
   if( *( temporaryObjectiveFunctionValuesInThread + temporaryIndividualIndex ) < *( temporaryObjectiveFunctionValuesInThread + individualIndex ) ){
    temporaryObjectiveFunctionValueInThread = *( temporaryObjectiveFunctionValuesInThread + temporaryIndividualIndex );
    *( temporaryObjectiveFunctionValuesInThread + temporaryIndividualIndex ) = *( temporaryObjectiveFunctionValuesInThread + individualIndex );
    *( temporaryObjectiveFunctionValuesInThread + individualIndex ) = temporaryObjectiveFunctionValueInThread;
    
    temporaryIndexOfIndividualInThread = *( temporaryIndexesOfIndividualsInThread + temporaryIndividualIndex );
    *( temporaryIndexesOfIndividualsInThread + temporaryIndividualIndex ) = *( temporaryIndexesOfIndividualsInThread + individualIndex );
    *( temporaryIndexesOfIndividualsInThread + individualIndex ) = temporaryIndexOfIndividualInThread;
   }
  }
 }
 //Sort the individuals using objective function values
 
 //Get the donor indexes
 for( int donorIndex = 0; donorIndex < NUMBEROFDONORS; donorIndex ++ ){
  *( indexesOfDonorsInThread + donorIndex ) = *( temporaryIndexesOfIndividualsInThread + donorIndex );
 }
 //Get the donor indexes
 
 //Get the receiver indexes
 for( int receiverIndex = 0; receiverIndex < NUMBEROFRECEIVERS; receiverIndex ++ ){
  *( indexesOfReceiversInThread + receiverIndex ) = *( temporaryIndexesOfIndividualsInThread + ( NUMBEROFINDIVIDUALSINTHREAD - 1 - receiverIndex ) );
 }
 //Get the receiver indexes
}
void* phases( void* rank ){
 srand( time( NULL ) );	
 long migrationRank = 0;	
 long threadRank = ( long ) rank;
 long neighborThreadRank = ( threadRank + 1 ) % NUMBEROFTHREADS;
 
 //Generate individuals and calculate objective function values
 long* evaluationIndexInThread = ( long* )malloc( sizeof( long ) );
 int* indexesOfDonorsInThread = ( int* )malloc( NUMBEROFDONORS * sizeof( int ) );
 int* indexesOfReceiversInThread = ( int* )malloc( NUMBEROFRECEIVERS * sizeof( int ) );
 double* objectiveFunctionValuesInThread = ( double* )malloc( NUMBEROFINDIVIDUALSINTHREAD * sizeof( double ) );
 double* individualsInThread = ( double* )malloc( NUMBEROFINDIVIDUALSINTHREAD * NUMBEROFPARAMETERS * sizeof( double ) );
 
 initializeIndividuals( individualsInThread );
 calculateObjectiveFunctionValues( individualsInThread, objectiveFunctionValuesInThread, OBJECTIVEFUNCTION );
 
 individuals[ threadRank ] = individualsInThread;
 indexesOfDonors[ threadRank ] = indexesOfDonorsInThread;
 evaluationIndexes[ threadRank ] = evaluationIndexInThread;
 indexesOfReceivers[ threadRank ] = indexesOfReceiversInThread;
 objectiveFunctionValues[ threadRank ] = objectiveFunctionValuesInThread;
 //Generate individuals and calculate objective function values
 
 //Get the best objective function value
 double* bestObjectiveFunctionValueInThread = bestObjectiveFunctionValues[ threadRank ];
 *( bestObjectiveFunctionValueInThread ) = getBestObjectiveFunctionValue( threadRank );
 printf( "Initial Best:%-6.4e\n", *( bestObjectiveFunctionValueInThread ) );
 //Get the best objective function value
 
 *( evaluationIndexInThread ) = 0;
 while( *( evaluationIndexInThread ) < MAXIMUMFITNESSEVALUATIONS ){
  immuneSystemPhase( threadRank );
  convalescentTreatmentPhase( threadRank );
  
  /*
  if( ( cycleIndex != 0 || cycleIndex != ( NUMBEROFCYCLES - 1 ) ) && ( ( cycleIndex + 1 ) % ( MIGRATIONPERIOD ) == 0 ) ){
   sem_t* currentEmigrantControlSemaphores = ( sem_t* )( ( char* )emigrantControlSemaphores + migrationRank * NUMBEROFTHREADS * sizeof( sem_t ) );
   setEmigrantFoodSource( threadRank, neighborThreadRank, migrationRank );
   sem_post( &currentEmigrantControlSemaphores[ neighborThreadRank ] );
   sem_wait( &currentEmigrantControlSemaphores[ threadRank ] );
   getEmigrantFoodSource( threadRank, migrationRank );
   migrationRank = migrationRank + 1; 
  }
  */

 }
 return NULL;
}
double calculatePressureVesselCost(double* x) {
    double cost = 0.6224 * x[0] * x[2] * x[3] +
        1.7781 * x[1] * pow(x[2], 2) +
        3.1661 * pow(x[0], 2) * x[3] +
        19.84 * pow(x[0], 2) * x[2];

    if (x[0] < 0.0193 * x[2]) cost += 1000000;
    if (x[1] < 0.00954 * x[2]) cost += 1000000;
    if ((PI * pow(x[2], 2) * x[3]) + (4.0 / 3.0 * PI * pow(x[2], 3)) > 1296000) cost += 1000000;
    if (x[3] > 240) cost += 1000000;

    return cost;
}
