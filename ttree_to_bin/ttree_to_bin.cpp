//Si detectors calibration by alpha source, and Phillips 7164.

#include <ctime>
#include <sys/stat.h>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <cmath>
#include <string>
#include "TFile.h"
#include "TH1.h"
#include "TH2.h"
#include "TTree.h"
#include "TObject.h"
#include "TSystem.h"
#include <cstdint>
#include <unistd.h>



#define LEN( array ) ( sizeof(array) / sizeof((array)[0]) )
#define PRINT_INT( var ) printf( "%s: %d\n", #var, (var) ) 
#define PRINT_STR( var ) printf( "%s: %s\n", #var, (var) )
#define PRINT_DOUBLE( var ) printf( "%s: %f\n", #var, (var) )
#define PRINT_LONG( var ) printf( "%s: %ld\n", #var, (var) )


// #define DEBUG  1
// #define USE_TEST_TREE  1

#define WRITE_FILES  1


// #define WRITE_ONE_FILE  1

// #define GET_MAX_FILE_SIZE 1



const int num_channels = 32;  // we have a 32x32 array of pixels on the detector.
const int BUFFER_SIZE = 64;
const int num_outfile_types = 2;
string outfile_types[ num_outfile_types ] = { "efront", "eback" } ;




using namespace std;

const char *test_tree = "deadlayerdet3rt.root";

int open_all_files( const char *rootfile, const char *dirname, ofstream outfiles[32][32][2] );
int write_all_files( const char *rootfile, const char *dirname );
void write_data( int det, int fstrip, int bstrip, const char *outdir,
		 int num_data, double data[][ BUFFER_SIZE ]  ) ;


    
// int close_all_files( ofstream outfiles[32][32][2]);
// int clear_all_files( const char *rootfile, const char *dirname );


void debug( TTree *tree, Int_t *singles_scalers, double *singles_energies );

int get_prefix( char *prefix, const char *rootfile );
void get_max_file_size( const char *outdir );

// int get_strip_num(int x);



int ttree_to_bin( const char *rootfile, const char *dirname )
{
#ifdef GET_MAX_FILE_SIZE
    get_max_file_size( dirname ) ;
#endif

    
    int ret = 0;
#ifdef USE_TEST_TREE
    ret = write_all_files( test_tree, dirname );
#else
    ret = write_all_files( rootfile, dirname );
#endif
    return ret ? 0 : -1; /// convert to unix standarod.
}







int write_all_files( const char *rootfile, const char *outdir )
{
    
    // get time 
    time_t start_time;
    time( &start_time );
    time_t current_time = start_time;

    int dets_used[4] = {0} ; 
    
    double buffered_data[4][32][32][ num_outfile_types ][ BUFFER_SIZE ];

    int buffered_data_used[4][32][32] = {{{0}}};
    
    // remove the .root suffix
    char prefix[64];
    get_prefix( prefix, rootfile );

    // open the tree for reading    
    TFile *fp = new TFile( rootfile );
    if( !fp )
    {
	printf( "ERRORR: unable to open file: %s\n", rootfile );
	return 0;
    }
    TTree *tree = (TTree *) fp->Get( "Singles_Tree" );
    if( ! tree )
    {
	puts( "ERROR: unable to obtain the requested tree from this file." );
	return 0;
    }
    

    Int_t global_scalers[9];
    Int_t singles_scalers[5];
    double singles_energy[2];
    Int_t *det;
    
    tree->SetBranchAddress( "Global_Scalers", &(global_scalers[0]) );
    tree->SetBranchAddress( "Singles_Scalers", &(singles_scalers[0]) );
    tree->SetBranchAddress( "Singles_Energy", &(singles_energy[0]) );
    // tree->SetBranchAddress( "Det", &det );
    
#ifdef DEBUG
    debug( tree, singles_scalers, singles_energy );
#endif

    
#ifdef WRITE_FILES

    // vars we need 
    int num_tenths_finished = 0;
    int num_entries = tree->GetEntries();

    printf( "num data: %d\n", num_entries ) ;
    	
    
    // initiate for loop through tree 
    for( int j=0; j<num_entries; j++)
    {
	
	// state progress
	if( j*1.0 / num_entries >= num_tenths_finished / 10.0 )
	{
	    time( &current_time );

	    printf( "%d/10 complete, ETA: %f mins\n", num_tenths_finished,
		    ( difftime(current_time, start_time) / 60.0 ) * (10 - num_tenths_finished )
		    / num_tenths_finished );

	    ++num_tenths_finished;
	}


	// write the data
	
	// populate the branch addresses 
	tree->GetEntry(j);

	// construct file  name and open stream 
	int fstrip = singles_scalers[2] - 1 ;
	int bstrip = singles_scalers[3] - 1 ;
	int det = singles_scalers[4] - 1 ;

	dets_used[ det ] = 1;

	int idx = buffered_data_used[ det ][ fstrip ][ bstrip ] ++;
	buffered_data[ det ][ fstrip ][ bstrip ][0][ idx ] = singles_energy[0];
	buffered_data[ det ][ fstrip ][ bstrip ][1][ idx ] = singles_energy[1];

	
	if( idx + 1 == BUFFER_SIZE )
	{
	    write_data( det, fstrip, bstrip, outdir, BUFFER_SIZE,
			& buffered_data[ det ][ fstrip ][ bstrip ][0] );
	    
	    buffered_data_used[ det ][ fstrip ][ bstrip ] = 0 ;
	}
	
	// double outdata[ num_outfile_types ] = { singles_energy[0], singles_energy[1] } ;
	    

    }
    fp->Close(); // close the root file


    // write the rest of the partially filled buffers
    for( int d = 0; d < 4; d++ )
    {
	for( int i = 0; i < 32; i++ )
	{
	    for( int j = 0; i < 32; i++ )
	    {
		int num_data = buffered_data_used[d][i][j];

		if( num_data )
			   write_data( d, i, j, outdir, num_data, buffered_data[d][i][j] );

	    }   
	}
    }


    // say which dets were used. this will make the users life easier
    // if not all 4 were used.
    ofstream dets_used_file;
    char fname[128] ;
    sprintf( fname, "%s/dets_used", outdir );
    dets_used_file.open( fname ) ;
    for( int i=0; i<4; i++ )
    {
	if( dets_used[i] )
	{
	    dets_used_file << i + 1 << std::endl;
	}
    }
    dets_used_file.close();
    
    return 1;
}









void write_data( int det, int fstrip, int bstrip, const char *outdir,
	int num_data, double data[][ BUFFER_SIZE ]  )
{

    ofstream outfile;

    // this shall hold the name of the file
    char tmp_file_name[128];

    
    for( int k=0; k<num_outfile_types; k++ )
    {
	    
	string outfile_type = outfile_types[ k ] ;
	

	sprintf( tmp_file_name, "%s%d/%d/%d/%s.bin", outdir, det + 1,
		 fstrip, bstrip, outfile_type.c_str()  );
	    
#ifdef WRITE_ONE_FILE
	PRINT_STR( tmp_file_name );
#endif


	// keep trying to open file until it is good.
	int q = 0 ;
	while (1)
	{
	    outfile.open( tmp_file_name, ios::out | ios::binary | ios::app );
	    if ( outfile.good() )
		break;
	    // printf( "WARNING: failed to open %s, trying again...\n", tmp_file_name ); 
	    outfile.close();

	    if( q++ > 10 )
	    {
		printf( "ERROR: failed to open %s 10 times consecutively, exiting", tmp_file_name );
		exit( 0 ) ;
	    }
	}

	

#ifdef DEBUG_FILE_OPEN
	if( ! outfile.good() )
	{
	    puts("ERROR: outfile bad before write" );
	}
	PRINT_LONG( (long) outfile.tellp() );
#endif
	outfile.write( (char *)  & data[ k ], sizeof(double) * num_data );
#ifdef DEBUG_FILE_OPEN
	PRINT_LONG( (long) outfile.tellp() );
#endif
	if( ! outfile.good() )
	{
	    puts("ERROR: write error.");
	}
	
// outfile.write( (char *) singles_scalers, 2*sizeof(Int_t) );
	// outfile.write( (char *) singles_scalers + 4, 
	
	// close file 
	outfile.close();
#ifdef WRITE_ONE_FILE
	if( j>=10) break;
#endif
    }
#endif  // WRITE_FILES
}









void debug( TTree *tree, Int_t *singles_scalers, double *singles_energies )
{
    tree->Print();
    tree->Show(0);
    tree->GetEntry(0);

    int fstrip = singles_scalers[2];
    int bstrip = singles_scalers[3];

    double efront = singles_energies[0];
    double eback = singles_energies[1];

    puts( "\nData we read: ") ;
    PRINT_INT( (int) sizeof(double) );
    PRINT_INT( fstrip );
    PRINT_INT( bstrip );
    PRINT_DOUBLE( efront );
    PRINT_DOUBLE( eback );
    puts("\n\n");
}








// populate char * with prefix of the .root file or a default if in test mode 
int get_prefix( char *prefix, const char *rootfile )
{
#ifndef USE_TEST_TREE
    const char *last_slash = strrchr( rootfile, '/' );
    if( ! last_slash ) last_slash = rootfile;
    strcpy( prefix, last_slash  );
    * strstr( prefix, "." ) = '\0';
#else
    strcpy( prefix, "test" );
#endif
    return 1;
}




#ifdef GET_MAX_FILE_SIZE 
void get_max_file_size( const char *outdir )
{

    char tmp_file_name[128];
    

    int i = 0 ;
    while(1)
    {
	ofstream outfile;
	sprintf( tmp_file_name, "%s/%d", outdir, i  );
	outfile.open( tmp_file_name, ios::out | ios::binary | ios::app );

	while (1)
	{
	    outfile.open( tmp_file_name, ios::out | ios::binary | ios::app );
	    if ( outfile.good() )
		break;
	    // printf( "WARNING: failed to open %s, trying again...\n", tmp_file_name ); 
	    outfile.close();
	    usleep( 1 ) ;
	}

	outfile.write( (char *) &i, sizeof( int ) ) ;
	PRINT_INT( i );
	i++;
	   
    }

}

    
#endif
