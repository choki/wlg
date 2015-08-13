#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h> //getline()
#include <string.h> //strtok()
#include <stdbool.h>	//boolean
#include "common.h"
#include "trace_parser.h"

// Each array is allocated to R/W separately.
// sum[0] : Read
// Sum[1] : Write
summary sum[2][SECTOR_ARRAY_SIZE] = {0};

void main(void)
{
    FILE *fpR1 = NULL;
    FILE *fpR2 = NULL;
    FILE *fpW = NULL;
    char *line1 = NULL;
    char *line2 = NULL;
    char *tmp = NULL;
    size_t len = 0;
    readLine string1;
    readLine string2;
    bool find = false;
    int i,j;
    int cur_bytes = 0;
    int total_count = 0;
    int r_skip;

    fpR1 = fopen(PARSER_INPUT_FILE_NAME, "r");
    fpR2 = fopen(PARSER_INPUT_FILE_NAME, "r");
    fpW = fopen(PARSER_OUTPUT_FILE_NAME, "w+");

    line1 = malloc(sizeof(char)*MAX_STR_LEN);
    line2 = malloc(sizeof(char)*MAX_STR_LEN);
    tmp = malloc(sizeof(char)*MAX_STR_LEN);

    while( getline(&line1, &len, fpR1) != -1 ){
	cur_bytes = ftell(fpR1);
	
	//Parsing
	parse_one_line(line1, &string1);
	
	//Only "D" action is eligible
	if( strcmp(string1.action,"D") == 0 ){
	    while( getline(&line2, &len, fpR2) != -1 ){
		//Parsing
		parse_one_line(line2, &string2);

		//Succeed to find
		if( string1.sSector == string2.sSector && \
			strcmp(string2.action,"C") == 0 ){
		    PRINT("Succeed\n");
		    PRINT("line1:%s", line1);
		    PRINT("line2:%s", line2);
		    find = true;
		    break;
		}
	    }//End of while()
	    //Fail to find
	    if(find == false){
		PRINT("Fail\n");
		PRINT("line1:%s", line1);
	    }
	    //Write back
	    line1[strlen(line1)-1] = '\0';
	    if(find == true){
		sprintf(tmp, "%s,%lf\n", line1, string2.sTime - string1.sTime);
		//Array update
		if( strstr(string2.rwbs,"R")!=NULL ){
		    //Read
		    sum[WG_READ][string2.size].total += string2.sTime - string1.sTime;
		    sum[WG_READ][string2.size].count ++;
		}else if( strstr(string2.rwbs,"W")!=NULL ){
		    //Write
		    sum[WG_WRITE][string2.size].total += string2.sTime - string1.sTime;
		    sum[WG_WRITE][string2.size].count ++;
		}
	    }else{
		sprintf(tmp, "%s,%s\n", line1, "Error");
	    }
	    PRINT("%s\n", tmp);
	    fwrite(tmp, 1, strlen(tmp), fpW);
	    //Re-initialize
	    find = false;
	    //In next time, search will be start from last fpR1's file position.
	    fseek(fpR2, cur_bytes, SEEK_SET);
	} //End of if()
    } //End of while()
    PRINT("\nThe END\n");
    
    //Print summary
    PRINT("\t\t\tSECTOR SIZE  =  TOTAL LAT./Cnt = AVG LAT.\n");
    PRINT("\t\t READ                                    WRITE\n");
    PRINT("----------------------------------------------------------------------------\n");

    for(i=0; i<=SECTOR_ARRAY_SIZE; i++){
	r_skip = 0;
	for(j=0; j<NUM_OPERATION_TYPE; j++){
	    total_count += sum[j][i].count;

	    if(sum[j][i].count == 0){ 
		if(j == WG_READ){
		    r_skip = 1;
		}else{
		    if(r_skip == 0){
			PRINT("\t|\n");
		    }else{
		    }
		}
		continue;
	    }else{
		if(j == WG_READ){
		    PRINT("%5d = %10lf/%4d = %10lf", 
			    i, sum[j][i].total, sum[j][i].count, sum[j][i].total/sum[j][i].count);
		}else{
		    if(r_skip == 1){
			PRINT("\t\t\t\t");
		    }	
    		    PRINT("\t|\t%5d = %10lf/%4d = %10lf\n", 
			    i, sum[j][i].total, sum[j][i].count, sum[j][i].total/sum[j][i].count);

		}
	    }
	}
    }
    PRINT("Total count = %d\n", total_count);
    fclose(fpR1);
    fclose(fpR2);
    fclose(fpW);
    free(line1);
    free(line2);
    free(tmp);
}

