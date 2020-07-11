/**************************************************
*  This program is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation, either version 3 of the License, or
*  (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program.  If not, see <https://www.gnu.org/licenses/>.
*
*  Author : Aalok Shah
*  Date : June - 20220
**********************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "kiss_fft.h"
#include "_kiss_fft_guts.h"
#include "xlsxwriter.h"

#define NZEROS_L 6
#define NPOLES_L 6
#define GAIN_L   2.488501358e+09

#define NUM_OF_SAMPLES 4096

#define NZEROS_H 3
#define NPOLES_H 3
#define GAIN_H   1.028032618e+00

#define NZEROS_B 6
#define NPOLES_B 6
#define GAIN_B   3.891452800e+05

#define SAMPLE_NUM 4096
#define SAMPLING_FREQUENCY 2272727.27

static float xv_h[NZEROS_H+1], yv_h[NZEROS_H+1];
static float xv_l[NZEROS_L+1], yv_l[NZEROS_L+1];
static float xv_b[NZEROS_B+1], yv_b[NZEROS_B+1];

static float Rawdata[NUM_OF_SAMPLES];
static float Highdata[NUM_OF_SAMPLES];
static float Lowdata[NUM_OF_SAMPLES];
static float Banddata[NUM_OF_SAMPLES];
static float HighLowdata[NUM_OF_SAMPLES];

static char *token,line[1024];
static unsigned int Rawcount=0;
static unsigned int row = 0, col = 0;

static lxw_workbook  *workbook = NULL;
static lxw_worksheet *worksheet = NULL;

/*******************************************  Filter Implementation ******************************************/
//Filter Code Generated Through:
//http://www-users.cs.york.ac.uk/~fisher/mkfilter/trad.html

/* BandPass Filter */;
static void filterband(unsigned int sampleno,float *data,float *out)    //3rd Order for Both High and Low Pass [Cutoff - 10k, 20k]
{
	unsigned int loop;

	for (loop=0;loop<sampleno;loop++)
	{
		xv_b[0] = xv_b[1]; xv_b[1] = xv_b[2]; xv_b[2] = xv_b[3]; xv_b[3] = xv_b[4]; xv_b[4] = xv_b[5]; xv_b[5] = xv_b[6];
		xv_b[6] = data[loop] / GAIN_B;
		yv_b[0] = yv_b[1]; yv_b[1] = yv_b[2]; yv_b[2] = yv_b[3]; yv_b[3] = yv_b[4]; yv_b[4] = yv_b[5]; yv_b[5] = yv_b[6];
		yv_b[6] =   (xv_b[6] - xv_b[0]) + 3 * (xv_b[2] - xv_b[4])
			+ ( -0.9462071191 * yv_b[0]) + (  5.7251799276 * yv_b[1])
			+ (-14.4384725600 * yv_b[2]) + ( 19.4264003990 * yv_b[3])
			+ (-14.7070671150 * yv_b[4]) + (  5.9401664637 * yv_b[5]);
		out[loop] = yv_b[6];
	}
}

static void filterhigh(unsigned int sampleno,float *data,float *out)    //3rd Order of High Pass [CutOff - 10k]
{
	unsigned int loop;
	for (loop=0;loop<sampleno;loop++)
	{
		xv_h[0] = xv_h[1]; xv_h[1] = xv_h[2]; xv_h[2] = xv_h[3];
		xv_h[3] = data[loop] / GAIN_H;
		yv_h[0] = yv_h[1]; yv_h[1] = yv_h[2]; yv_h[2] = yv_h[3];
		yv_h[3] =   (xv_h[3] - xv_h[0]) + 3 * (xv_h[1] - xv_h[2])
			+ (  0.9462071191 * yv_h[0]) + ( -2.8909374056 * yv_h[1])
			+ (  2.9447097288 * yv_h[2]);
		out[loop] = yv_h[3];
	}
}

static void filterlow(unsigned int sampleno,float *data,float *out)       //6td Order of Low Pass [CutOff - 20k]
{
	unsigned int loop;
	for (loop=0;loop<sampleno;loop++)
	{
		xv_l[0] = xv_l[1]; xv_l[1] = xv_l[2]; xv_l[2] = xv_l[3]; xv_l[3] = xv_l[4]; xv_l[4] = xv_l[5]; xv_l[5] = xv_l[6];
		xv_l[6] = data[loop] / GAIN_L;
		yv_l[0] = yv_l[1]; yv_l[1] = yv_l[2]; yv_l[2] = yv_l[3]; yv_l[3] = yv_l[4]; yv_l[4] = yv_l[5]; yv_l[5] = yv_l[6];
		yv_l[6] =   (xv_l[0] + xv_l[6]) + 6 * (xv_l[1] + xv_l[5]) + 15 * (xv_l[2] + xv_l[4])
			+ 20 * xv_l[3]
			+ ( -0.8076176684 * yv_l[0]) + (  5.0182346822 * yv_l[1])
			+ (-12.9954332470 * yv_l[2]) + ( 17.9530082240 * yv_l[3])
			+ (-13.9545653480 * yv_l[4]) + (  5.7863733315 * yv_l[5]);
		out[loop] = yv_l[6];
	}
}

/********************************************* FFT Implementation ***********************************************/

void vKissFft(float * data, kiss_fft_cpx *hrList, unsigned int col_num)
{
	float ftemp, fdiff;
	unsigned int u16upperIndex, u16lowerIndex;
	kiss_fft_cfg fftCfg;
	kiss_fft_cpx fftIn[SAMPLE_NUM], fftOut[SAMPLE_NUM];
	kiss_fft_cpx fundamentalFreq, *harmonicIndex = hrList;
	unsigned int u16fftSampleNo = (Rawcount%2)!=0 ? (1024) : Rawcount;
	unsigned int loop = 0;

	fftCfg = kiss_fft_alloc(u16fftSampleNo, 0, 0, 0);
	if ( fftCfg  == NULL)
	{
		printf("Memory allocation failed for fftCfg");
		return;
	}

	for (loop = 0; loop < u16fftSampleNo; loop++)
	{
		fftIn[loop].r = data[loop];
		fftIn[loop].i = 0;
	}
	kiss_fft(fftCfg, fftIn, fftOut);
	free(fftCfg);

	row = 1;
	for (loop = 0; loop < u16fftSampleNo; loop++)
	{
		fftIn[loop].r = (float)((float)(loop * SAMPLING_FREQUENCY) / u16fftSampleNo);
		ftemp = 0;
		ftemp += fftOut[loop].r * fftOut[loop].r;
		ftemp += fftOut[loop].i * fftOut[loop].i;
		ftemp = (float)sqrt(ftemp);
		fftIn[loop].i = (float) ((float)(2 * ftemp) / u16fftSampleNo);

		worksheet_write_number(worksheet, row, col_num, round(fftIn[loop].r * 1000)/1000, NULL);
		worksheet_write_number(worksheet, row, col_num + 1, round(fftIn[loop].i * 1000)/1000, NULL);
		row++;
	}

}

/**************************************************************** Main Function ******************************************/
int main()
{
	char *token = NULL, line[1024];
	float temp;

	FILE *finput = fopen("input.csv","r");
	if(finput == NULL)
	{
		printf("\n input file opening failed ");
		return -1 ;
	}

	/* Create a workbook and add a worksheet. */
	workbook  = workbook_new("output.xlsx");
	worksheet = workbook_add_worksheet(workbook, "output");

	printf("Starting Conversion : input.csv => output.xlsx\n");

	while(fgets(line, sizeof(line), finput) != NULL && Rawcount < NUM_OF_SAMPLES)
	{
		token = strtok(line, ",");
		while(token !=NULL)
		{
			Rawdata[Rawcount++]=atof(token);
			token = strtok(NULL, ",");
		}
	}
	fclose(finput);
	printf("Samples Considered for FILTER : %d\n",Rawcount);

	for (row = 1; row <= NUM_OF_SAMPLES; row++) {
	}

	col = 0;
	row = 0;
	worksheet_write_string(worksheet, row, col, "RAW_DATA", NULL);
	for (row = 1; row <= NUM_OF_SAMPLES; row++) {
		worksheet_write_number(worksheet, row, col, Rawdata[row - 1], NULL);
	}

	lxw_chart *chart_r = workbook_add_chart(workbook, LXW_CHART_LINE);
	/* Add the first series to the chart. */
	lxw_chart_series *series_r = chart_add_series(chart_r, NULL, "=output!$A$2:$A$4097");
	/* Set the name for the series instead of the default "Series 1". */
	//chart_series_set_name(series_r, "=output!$A$1");
	/* Add a chart title and some axis labels. */
	chart_title_set_name(chart_r,        "Raw Data");
	chart_axis_set_name(chart_r->x_axis, "Data Points");
	chart_axis_set_name(chart_r->y_axis, "ADC Count");
	/* Set an Excel chart style. */
	//Note : Values Possible - 1 to 48
	chart_set_style(chart_r, 26);
	/* Insert the chart into the worksheet. */
	worksheet_insert_chart(worksheet, CELL("K1"), chart_r);
	chart_series_set_marker_type(series_r, LXW_CHART_MARKER_NONE);
	//chart_series_set_marker_size(series_r, 3);

	col = 1;
	row = 0;
	worksheet_write_string(worksheet, row, col, "RAW=>HighPass", NULL);
	filterhigh(NUM_OF_SAMPLES, Rawdata, Highdata);
	for (row=1;row<=NUM_OF_SAMPLES;row++)
	{
		worksheet_write_number(worksheet, row, col, round(Highdata[row - 1] * 1000)/1000, NULL);
	}

	lxw_chart *chart_h = workbook_add_chart(workbook, LXW_CHART_LINE);
	/* Add the first series to the chart. */
	lxw_chart_series *series_h = chart_add_series(chart_h, NULL, "=output!$B$2:$B$4097");
	/* Set the name for the series instead of the default "Series 1". */
	//chart_series_set_name(series_h, "=output!$B$1");
	/* Add a chart title and some axis labels. */
	chart_title_set_name(chart_h,        "Raw=>HighPass");
	chart_axis_set_name(chart_h->x_axis, "Data Points");
	chart_axis_set_name(chart_h->y_axis, "ADC Count");
	/* Set an Excel chart style. */
	chart_set_style(chart_h, 24);
	/* Insert the chart into the worksheet. */
	worksheet_insert_chart(worksheet, CELL("K18"), chart_h);
	chart_series_set_marker_type(series_h, LXW_CHART_MARKER_NONE);
	//chart_series_set_marker_size(series_h, 3);

	col = 2;
	row = 0;
	worksheet_write_string(worksheet, row, col, "RAW=>LowPass", NULL);
	filterlow(NUM_OF_SAMPLES, Rawdata, Lowdata);
	for (row=1;row<=NUM_OF_SAMPLES;row++)
	{
		worksheet_write_number(worksheet, row, col, round(Lowdata[row - 1] * 1000)/1000, NULL);
	}

	lxw_chart *chart_l = workbook_add_chart(workbook, LXW_CHART_LINE);
	/* Add the first series to the chart. */
	lxw_chart_series *series_l = chart_add_series(chart_l, NULL, "=output!$C$2:$C$4097");
	/* Set the name for the series instead of the default "Series 1". */
	//chart_series_set_name(series_l, "=output!$C$1");
	/* Add a chart title and some axis labels. */
	chart_title_set_name(chart_l,        "Raw=>LowPass");
	chart_axis_set_name(chart_l->x_axis, "Data Points");
	chart_axis_set_name(chart_l->y_axis, "ADC Count");
	/* Set an Excel chart style. */
	chart_set_style(chart_l, 23);
	/* Insert the chart into the worksheet. */
	worksheet_insert_chart(worksheet, CELL("K36"), chart_l);
	chart_series_set_marker_type(series_l, LXW_CHART_MARKER_NONE);
	//chart_series_set_marker_size(series_l, 3);

	col = 3;
	row = 0;
	worksheet_write_string(worksheet, row, col, "HighPass=>LowPass", NULL);
	filterlow(NUM_OF_SAMPLES, Highdata, HighLowdata);
	for (row=1;row<=NUM_OF_SAMPLES;row++)
	{
		worksheet_write_number(worksheet, row, col, round(HighLowdata[row - 1] * 1000)/1000, NULL);
	}

	lxw_chart *chart_hl = workbook_add_chart(workbook, LXW_CHART_LINE);
	/* Add the first series to the chart. */
	lxw_chart_series *series_hl = chart_add_series(chart_hl, NULL, "=output!$D$2:$D$4097");
	/* Set the name for the series instead of the default "Series 1". */
	//chart_series_set_name(series_hl, "=output!$D$1");
	/* Add a chart title and some axis labels. */
	chart_title_set_name(chart_hl,        "HighPass=>LowPass");
	chart_axis_set_name(chart_hl->x_axis, "Data Points");
	chart_axis_set_name(chart_hl->y_axis, "ADC Count");
	/* Set an Excel chart style. */
	chart_set_style(chart_hl, 22);
	/* Insert the chart into the worksheet. */
	worksheet_insert_chart(worksheet, CELL("K54"), chart_hl);
	chart_series_set_marker_type(series_hl, LXW_CHART_MARKER_NONE);
	//chart_series_set_marker_size(series_hl, 3);

	col = 4;
	row = 0;
	worksheet_write_string(worksheet, row, col, "RAW=>BandPass", NULL);
	filterband(NUM_OF_SAMPLES, Rawdata, Banddata);
	for (row=1;row<=NUM_OF_SAMPLES;row++)
	{
		worksheet_write_number(worksheet, row, col, round(Banddata[row - 1] * 1000)/1000, NULL);
	}

	lxw_chart *chart_b = workbook_add_chart(workbook, LXW_CHART_LINE);
	/* Add the first series to the chart. */
	lxw_chart_series *series_b = chart_add_series(chart_b, NULL, "=output!$E$2:$E$4097");
	/* Set the name for the series instead of the default "Series 1". */
	//chart_series_set_name(series_b, "=output!$E$1");
	/* Add a chart title and some axis labels. */
	chart_title_set_name(chart_b,        "Raw=>BandPass");
	chart_axis_set_name(chart_b->x_axis, "Data Points");
	chart_axis_set_name(chart_b->y_axis, "ADC Count");
	/* Set an Excel chart style. */
	chart_set_style(chart_b, 21);
	/* Insert the chart into the worksheet. */
	worksheet_insert_chart(worksheet, CELL("K72"), chart_b);
	chart_series_set_marker_type(series_b, LXW_CHART_MARKER_NONE);
	//chart_series_set_marker_size(series_b, 3);

	printf("Filter Generation Completed!\n");

	col = 5; //5 and 6 : Would be used
	row = 0;
	worksheet_write_string(worksheet, row, col, "FREQ", NULL);
	worksheet_write_string(worksheet, row, col + 1, "MAG (RAW)", NULL);
	vKissFft(Rawdata,0,col);

	lxw_chart *chart_fftr = workbook_add_chart(workbook, LXW_CHART_SCATTER_STRAIGHT_WITH_MARKERS);
	/* Add the first series to the chart. */
	lxw_chart_series *series_fftr = chart_add_series(chart_fftr, "=output!$F$2:$F$201", "=output!$G$2:$G$201");
	/* Set the name for the series instead of the default "Series 1". */
	//chart_series_set_name(series_fftr, "=output!$G$1");
	/* Add a chart title and some axis labels. */
	chart_title_set_name(chart_fftr,        "FFT (Raw)");
	chart_axis_set_name(chart_fftr->x_axis, "Frequency");
	chart_axis_set_name(chart_fftr->y_axis, "Magnitude");
	/* Set an Excel chart style. */
	chart_set_style(chart_fftr, 20);
	/* Insert the chart into the worksheet. */
	worksheet_insert_chart(worksheet, CELL("K90"), chart_fftr);
	chart_series_set_marker_type(series_fftr, LXW_CHART_MARKER_DIAMOND);
	chart_series_set_marker_size(series_fftr, 5);


	col = 7; //7 and 8 : Would be used
	row = 0;
	worksheet_write_string(worksheet, row, col, "FREQ", NULL);
	worksheet_write_string(worksheet, row, col + 1, "MAG (High=>Low)", NULL);
	vKissFft(HighLowdata,0,col);

	lxw_chart *chart_fftlh = workbook_add_chart(workbook, LXW_CHART_SCATTER_STRAIGHT_WITH_MARKERS);
	/* Add the first series to the chart. */
	lxw_chart_series *series_fftlh = chart_add_series(chart_fftlh, "=output!$H$2:$H$201", "=output!$I$2:$I$201");
	/* Set the name for the series instead of the default "Series 1". */
	//chart_series_set_name(series_fftlh, "=output!$I$1");
	/* Add a chart title and some axis labels. */
	chart_title_set_name(chart_fftlh,        "FFT (HighPass=>LowPass)");
	chart_axis_set_name(chart_fftlh->x_axis, "Frequency");
	chart_axis_set_name(chart_fftlh->y_axis, "Magnitude");
	/* Set an Excel chart style. */
	chart_set_style(chart_fftlh, 19);
	/* Insert the chart into the worksheet. */
	worksheet_insert_chart(worksheet, CELL("K108"), chart_fftlh);
	chart_series_set_marker_type(series_fftlh, LXW_CHART_MARKER_DIAMOND);
	chart_series_set_marker_size(series_fftlh, 5);

	printf("FFT Generation Completed!\n");
	workbook_close(workbook);

	return 0;
}


