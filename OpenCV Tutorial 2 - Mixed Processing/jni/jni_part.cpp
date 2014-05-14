#include <jni.h>
#include <vector>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>
#include <fstream>

#include <string>

using namespace std;
using namespace cv;

//-------------------
#define IMG_SIZE_X (320)  //原始檔寬
#define IMG_SIZE_Y (240)  //原始檔長
#define min(a,b) ( (a<b) ? a : b )
#define max(a,b) ( (a>b) ? a : b )
#define WHITE_DEFAT (255)

#define sw (1+1)          //silding windows

//data
unsigned long V_BGR[3][IMG_SIZE_X];  //Vertical  :垂直
unsigned long H_BGR[3][IMG_SIZE_Y];  //Horizontal:水平

int Grid_X1=100;      //X軸格線起點  //70
int Grid_X2=220;     //X軸格線終點  //210
int Grid_Y1=100;      //Y軸格線起點  //130
int Grid_Y2=180;     //Y軸格線終點  //200


int Digital[25];
int VPoint[100]= {}, V_tmp=0;
int HPoint[100]= {}, H_tmp=0;
bool ChangeAlert = false;
//---j
//UI
IplImage* frame;       //原始圖
IplImage* EndImage1;   //主畫面
IplImage* EndImage2;
IplImage* EndImage3;
IplImage* tmp;
IplImage* tmp1;
IplImage* img;
IplImage* img1;
//---------------------------------------
IplImage* Digital_Img[8];        //數字辨識圖
//---------------------------------------
CvCapture* capture;
//long* Array_KsumISub;
//int* Array_ISubTemp;

//暫存檔
int nRow=0,nCol=0;
int SmoothOffset_V=1;         //-n..點..+n 累加取平均 Smooth:平滑 Vertical
int SmoothOffset_H=1;         //-n..點..+n 累加取平均 Smooth:平滑 Horizontal
int thrDarkGray;
IplImage* ImgTmpBGR;
IplImage* ImgTmpYUV;

int Grid_Point[8][4]={0}, number1=0;

void Grid(IplImage* pSrc, unsigned long point_X_beg, unsigned long point_X_end,
          unsigned long point_Y_beg, unsigned long point_Y_end, CvScalar color)
{
    CvPoint FromP,ToP;

    FromP = cvPoint(point_X_beg-1,point_Y_beg-1);  //最上
    ToP   = cvPoint(point_X_end-1,point_Y_beg-1);
    cvLine(pSrc, FromP, ToP, color, 1);
    FromP = cvPoint(point_X_beg-1,point_Y_end-1);  //最下
    ToP   = cvPoint(point_X_end-1,point_Y_end-1);
    cvLine(pSrc, FromP, ToP, color, 1);
    FromP = cvPoint(point_X_beg-1,point_Y_beg-1);  //最左
    ToP   = cvPoint(point_X_beg-1,point_Y_end-1);
    cvLine(pSrc, FromP, ToP, color, 1);
    FromP = cvPoint(point_X_end-1,point_Y_beg-1);  //最右
    ToP   = cvPoint(point_X_end-1,point_Y_end-1);
    cvLine(pSrc, FromP, ToP, color, 1);
}
void Smooth(int Smooth_Offset, int IMG_SIZE, int VH)
{
    const int Smooth_Width = 2*Smooth_Offset + 1;
    for(int i=Smooth_Offset; i<IMG_SIZE-Smooth_Offset; i++)
    {
        unsigned long Blue=0, Green=0, Red=0;
        for(int l=i-Smooth_Offset; l<=i+Smooth_Offset; l++)
        {
            Blue += VH==1 ? V_BGR[0][l]: H_BGR[0][l];
            Green+= VH==1 ? V_BGR[1][l]: H_BGR[1][l];
            Red  += VH==1 ? V_BGR[2][l]: H_BGR[2][l];
        }
        VH==1 ? V_BGR[0][i]=Blue /Smooth_Width : H_BGR[0][i]=Blue /Smooth_Width;
        VH==1 ? V_BGR[1][i]=Green/Smooth_Width : H_BGR[1][i]=Green/Smooth_Width;
        VH==1 ? V_BGR[2][i]=Red  /Smooth_Width : H_BGR[2][i]=Red  /Smooth_Width;
    }
}
int thrDarkGray_value(IplImage* pSrc, int VStart, int VEnd, int HStart, int HEnd)
{
    unsigned long Tmp=0;
    unsigned long nThrPixel = 0;
    unsigned long threshold=0;
    // Calculate thrDarkGray
    for(int x=VStart; x<=VEnd; x++)
    {
        for(int y=HStart; y<=HEnd; y++)
        {
            nThrPixel++;
            for (int c=0; c<pSrc->nChannels; c++)
            Tmp += (unsigned char)pSrc->imageData[ y*pSrc->widthStep + x*pSrc->nChannels + c ];
        }
    }
    Tmp /= nThrPixel;
    threshold += Tmp;
    threshold /= pSrc->nChannels;

    return threshold;
}

void pixel_link(int tmp[IMG_SIZE_X][IMG_SIZE_Y], int v1, int v2, int h1, int h2, int x1, int y1, int label)
{
    tmp[x1][y1]=label;
    while(1)
    {
        int nPixel=0, left_x, right_x, up_y, down_y;
        for(int x=v1; x<v2; x++)
        {
            for(int y=h1; y<h2; y++)
            {
                if(tmp[x][y] == label)
                {
                    left_x   = x-1;
                    right_x  = x+1;
                    up_y     = y-1;
                    down_y   = y+1;

                    if(left_x  < v1)   left_x  = v1;
                    if(right_x > v2-1) right_x = v2-1;
                    if(up_y < h1)      up_y    = h2;
                    if(down_y > h2-1)  down_y  = h2-1;


                    if(tmp[left_x][up_y] == 0xff)    //x-1,y-1
                    {
                        tmp[left_x][up_y] = label;
                        nPixel++;
                    }
                    if(tmp[right_x][up_y] == 0xff)   //x+1,y-1
                    {
                        tmp[right_x][up_y] = label;
                        nPixel++;
                    }
                    if(tmp[right_x][down_y] == 0xff) //x+1,y+1
                    {
                        tmp[right_x][down_y] = label;
                        nPixel++;
                    }

                    if(tmp[left_x][y] == 0xff)       //x-1,y
                    {
                        tmp[left_x][y] = label;
                        nPixel++;
                    }
                    if(tmp[x][up_y] == 0xff)         //x,y-1
                    {
                        tmp[x][up_y] = label;
                        nPixel++;
                    }
                    if(tmp[x][down_y] == 0xff)       //x,y+1
                    {
                        tmp[x][down_y] = label;
                        nPixel++;
                    }
                    if(tmp[right_x][y] == 0xff)      //x+1,y
                    {
                        tmp[right_x][y] = label;
                        nPixel++;
                    }
                }
            }
        }
        if( nPixel==0 ) break;
    }
}
void White_Area(IplImage* img, int* m, int i, int x_start, int x_end, int y_start, int y_end)
{
	for(int x=x_start; x<x_end; x++)
		for(int y=y_start; y<y_end; y++)
			if( (unsigned char)img->imageData[y*img->widthStep + x*img->nChannels + 0] == (unsigned char)0xff )
				m[i]++;
}

int thrDarkGray_value_y(IplImage* pSrc, int VStart, int VEnd, int HStart, int HEnd)
{
	unsigned long Tmp=0;
	unsigned long nThrPixel = 0;
	unsigned long threshold = 0;
	// Calculate thrDarkGray
	for(int y=HStart; y<=HEnd; y++)
	{
		for(int x=VStart; x<=VEnd; x++)
		{
			nThrPixel++;
			for (int c=0; c<pSrc->nChannels; c++)
				Tmp += (unsigned char)pSrc->imageData[ y*pSrc->widthStep + x*pSrc->nChannels + c ];
		}
	}
	Tmp /= nThrPixel;
	threshold += Tmp;
	threshold /= pSrc->nChannels;

	return threshold;
}
//-------------------
extern "C" {
//JNIEXPORT bool JNICALL Java_org_opencv_samples_tutorial2_Tutorial2Activity_FindLaneLines(
//		JNIEnv*, jobject, jlong addrGray, jlong addrRgb, jint nHorizonY, jint FrameNum, jint Alert);
JNIEXPORT jint JNICALL Java_org_opencv_samples_tutorial2_Tutorial2Activity_FindLaneLines(
		JNIEnv* env, jobject, jlong addrGray, jlong addrRgb, jint nHorizonY, jint FrameNum, jint Alert);
		//jlong file0, jlong file1, jlong file2, jlong file3, jlong file4, jlong file5,
		//jlong file6, jlong file7, jlong file8, jlong file9);


JNIEXPORT jint JNICALL Java_org_opencv_samples_tutorial2_Tutorial2Activity_FindLaneLines
	(JNIEnv* env, jobject, jlong addrGray, jlong addrRgb, jint nHorizonY, jint FrameNum, jint Alert)
	//		jlong file0, jlong file1, jlong file2, jlong file3, jlong file4, jlong file5,
	//		jlong file6, jlong file7, jlong file8, jlong file9)
{
	CvPoint FromPoint1,ToPoint1;
	ChangeAlert = true;

	EndImage1 = cvCreateImage(cvSize( IMG_SIZE_X, IMG_SIZE_Y ),8,3);
	EndImage2 = cvCreateImage(cvSize( IMG_SIZE_X, IMG_SIZE_Y ),8,3);

	//Grayimg = cvCreateImage(cvSize( IMAGE_WIDTH, IMAGE_HEIGHT ),8,1);

	// Get "Mat" data structure
	//Mat& matGray = *(Mat*)addrGray;
    Mat& matRGB  = *(Mat*)addrRgb;
	IplImage OriPic = matRGB;

	frame = &OriPic;

	int number=1;
	char buffer[9999];

	cvSetImageROI(frame, cvRect(0, frame->height*.9, frame->width, frame->height));
	cvSet(frame, cvScalar(0, 0, 0));
	cvResetImageROI(frame);
	cvResize(frame, EndImage1);
	//-------------------------
	//初始化
	ImgTmpBGR = cvCloneImage(EndImage1);
	//         原始檔 轉換後圖檔  RGB轉YCrCb
	cvCvtColor(EndImage1, EndImage1, CV_BGR2YCrCb);
	ImgTmpYUV = cvCloneImage(EndImage1);
    //---------------------------
	unsigned long Tmp[3]= {};
	unsigned long nThrPixel = 0;
	const int position_offset_V = 3;
	const int position_offset_H = 3;

	// Calculate thrDarkGray
	for ( int y=Grid_Y1+20; y<Grid_Y2-20; y++ )
	{
		for ( int x=Grid_X1+20; x<Grid_X2-20; x++ )
		{
			nThrPixel++;
			for ( int c=0; c<ImgTmpYUV->nChannels; c++ )
				Tmp[c] += (unsigned char)ImgTmpYUV->imageData[ y*ImgTmpYUV->widthStep + x*ImgTmpYUV->nChannels + c ];
		}
	}
	thrDarkGray = 0;
	for ( int c=0; c<ImgTmpYUV->nChannels; c++ )
	{
		Tmp[c] /= nThrPixel;
		thrDarkGray += Tmp[c];
	}
	thrDarkGray /= ImgTmpYUV->nChannels;
	//------------------------------
	//存成陣列 V_BGR: 0:Blue 1:Green 2:Red
	for(int i=0; i<IMG_SIZE_X; i++)
	        V_BGR[0][i]=V_BGR[1][i]=V_BGR[2][i]=255;

	// Min
	nRow=1;
	for(int y=Grid_Y1-1; y<Grid_Y2; y++)
	{
		int TmpBGR[3] = {}, TmpYUV[3] = {};
		for(int x=(Grid_X1-1); x<Grid_X2; x++)
		{
			for ( int c=0; c<ImgTmpYUV->nChannels; c++ )
			{
				TmpBGR[c] = ImgTmpBGR->imageData[y*ImgTmpBGR->widthStep + x*ImgTmpBGR->nChannels + c];
				TmpYUV[c] = ImgTmpYUV->imageData[y*ImgTmpYUV->widthStep + x*ImgTmpYUV->nChannels+c];
			}
			if( TmpYUV[0] < thrDarkGray*.6 )  //.6
				V_BGR[0][x] = min(V_BGR[0][x], ImgTmpYUV->imageData[ y*ImgTmpYUV->widthStep + x*ImgTmpYUV->nChannels ]);
		}
	}

	//平滑化 V     起點      圖片size     V
	Smooth(SmoothOffset_V, IMG_SIZE_X, 1);

	//畫折線圖:Vertical Blue
	unsigned long V_BGRMin = 255;
	//cvSet(Image, cvScalar(0,0,0));
	for(int x=0; x<IMG_SIZE_X; x++)
		if( x>=(Grid_X1-position_offset_V*2) && x<(Grid_X2+position_offset_V*2) )
			if ( V_BGRMin > V_BGR[0][x] )
				V_BGRMin = V_BGR[0][x];

	/*
	for(int x=0; x<IMG_SIZE_X; x++)
	{
		FromPoint1 = cvPoint(x  ,(unsigned char)(V_BGR[0][x]));
		ToPoint1   = cvPoint(x+1,(unsigned char)(V_BGR[0][x+1]));
		cvLine(EndImage1, FromPoint1, ToPoint1, CV_RGB(255,0,255), 1);
	}
	*/

	//動態選取範圍 V
	int VPoint[100]= {};
	int V_tmp=0;

	for(int x=Grid_X1; x-1<Grid_X2; x++)
	{
		const int value = (int)((thrDarkGray - (int)V_BGRMin) *.4);
		if ( ((long)V_BGR[0][x-position_offset_V] - (long)V_BGR[0][x]) > value )
		{
			if ( (int)V_BGR[0][x-1] < thrDarkGray &&
				 (int)V_BGR[0][x  ] < thrDarkGray &&
				 (int)V_BGR[0][x+1] < thrDarkGray
				 ){
				VPoint[V_tmp] = x-position_offset_V;
				V_tmp++;
			}
		}

		if ( ((long)V_BGR[0][x+position_offset_V] - (long)V_BGR[0][x]) > value )
		{
			if ( (int)V_BGR[0][x-1] < thrDarkGray &&
				 (int)V_BGR[0][x  ] < thrDarkGray &&
				 (int)V_BGR[0][x+1] < thrDarkGray
				 ){
				VPoint[V_tmp] = x+position_offset_V;
				V_tmp++;
			}
		}
	}
	//----------------------------
	//存成陣列 H_BGR: 0:Blue 1:Green 2:Red
	for(int i=0; i<IMG_SIZE_Y; i++)
	{
		if(i>=(Grid_Y1-position_offset_H*2) && i<(Grid_Y2+position_offset_H*2))
			H_BGR[0][i]=H_BGR[1][i]=H_BGR[2][i]=0;
		else
			H_BGR[0][i]=H_BGR[1][i]=H_BGR[2][i]=0;
	}

	// 共幾條線
	nCol=1;
	for ( int y=Grid_Y1-1; y<Grid_Y2; y++ )
	{
		int nNumDarkPixel = 0;
		for ( int x=Grid_X1-1; x<Grid_X2; x++ )
		{
			int TmpBGR[3] = {}, TmpYUV[3] = {};
			int value=thrDarkGray*.9;
			for ( int c=0; c<ImgTmpYUV->nChannels; c++ )
			{
				TmpBGR[c] = (unsigned char)ImgTmpBGR->imageData[y*ImgTmpBGR->widthStep + x*ImgTmpBGR->nChannels + c];
				TmpYUV[c] = (unsigned char)ImgTmpYUV->imageData[y*ImgTmpYUV->widthStep + x*ImgTmpYUV->nChannels + c];
			}
			if( TmpYUV[0] < thrDarkGray*.8 &&
				TmpBGR[0]<value && TmpBGR[1]<value && TmpBGR[2]<value
				){
				//min
				H_BGR[0][y]= nNumDarkPixel++;
				for(int c=0; c<ImgTmpYUV->nChannels; c++)  //Color:255
					EndImage1->imageData[y*ImgTmpYUV->widthStep + x*ImgTmpYUV->nChannels + c] = 0xff;
	        }
			else
				// Not dark gray
				for(int c=0; c<ImgTmpYUV->nChannels; c++)  //Color:0
					EndImage1->imageData[y*ImgTmpYUV->widthStep + x*ImgTmpYUV->nChannels + c] = 0;
	        }
	}

	//平滑化 H  起點Y         圖片size  H
	Smooth(SmoothOffset_H, IMG_SIZE_Y, 1);

	/*
	//畫折線圖:Horizontal Blue
	for(int y=0; y<IMG_SIZE_Y; y++)
	{
		FromPoint1 = cvPoint((unsigned char)(H_BGR[0][y]),y);
		ToPoint1   = cvPoint((unsigned char)(H_BGR[0][y+1]),y+1);
		cvLine(EndImage1,FromPoint1,ToPoint1,CV_RGB(255,255,0),1);
	}
	*/

	//動態選取範圍 H
	int HPoint[100]= {};
	int H_tmp=0;
	for(int y=Grid_Y1+1; y-1<Grid_Y2; y++){
	    if(((long)H_BGR[0][y] - (long)H_BGR[0][y-position_offset_H] > thrDarkGray*.1 ) &&
	    	(long)H_BGR[0][y-position_offset_H]<=3 && H_tmp==0
	    	){
	    	HPoint[H_tmp] = y-position_offset_H;
	    	H_tmp++;
	    }
		if( (int)H_BGR[0][y]<=3 && H_tmp>0 )
		{
			HPoint[H_tmp] = y+position_offset_H;
			H_tmp++;
			break;
	    }
	}

	//再次處理影像
	//存成陣列 V_BGR: 0:Blue 1:Green 2:Red
	V_BGRMin = 255;
	for(int x=0; x<IMG_SIZE_X; x++)
		V_BGR[0][x]=V_BGR[1][x]=V_BGR[2][x]=0;//0xff;

	cvSet(EndImage1, cvScalar(0,0,0));
	cvSet(EndImage2, cvScalar(0,0,0));
	//框架切割後顯示數字
	//框架外
	for(int x=0; x<Grid_X1; x++)
		for(int y=0; y<ImgTmpBGR->height; y++)
			for(int c=0; c<ImgTmpBGR->nChannels; c++)
				EndImage1->imageData[y*EndImage1->widthStep+x*EndImage1->nChannels+c]=ImgTmpBGR->imageData[y*ImgTmpBGR->widthStep+x*ImgTmpBGR->nChannels+c];

	for(int x=Grid_X2+1; x<ImgTmpBGR->width; x++)
		for(int y=0; y<ImgTmpBGR->height; y++)
			for(int c=0; c<ImgTmpBGR->nChannels; c++)
				EndImage1->imageData[y*EndImage1->widthStep+x*EndImage1->nChannels+c]=ImgTmpBGR->imageData[y*ImgTmpBGR->widthStep+x*ImgTmpBGR->nChannels+c];

	for(int y=0; y<Grid_Y1; y++)
		for(int x=0; x<ImgTmpBGR->width; x++)
			for(int c=0; c<ImgTmpBGR->nChannels; c++)
				EndImage1->imageData[y*EndImage1->widthStep+x*EndImage1->nChannels+c]=ImgTmpBGR->imageData[y*ImgTmpBGR->widthStep+x*ImgTmpBGR->nChannels+c];

	for(int y=Grid_Y2+1; y<ImgTmpBGR->height; y++)
		for(int x=0; x<ImgTmpBGR->width; x++)
			for(int c=0; c<ImgTmpBGR->nChannels; c++)
				EndImage1->imageData[y*EndImage1->widthStep+x*EndImage1->nChannels+c]=ImgTmpBGR->imageData[y*ImgTmpBGR->widthStep+x*ImgTmpBGR->nChannels+c];

	//動態選取範圍>0
	if(VPoint[0]>0 && VPoint[V_tmp-1]>0 && HPoint[0]>0 && HPoint[H_tmp-1]>0)
	{
		int V_Start=VPoint[0];
		int V_End  =VPoint[V_tmp-1];
		int H_Start=HPoint[0];
		int H_End  =HPoint[H_tmp-1];

		for(int x=0; x<IMG_SIZE_X; x++)
			V_BGR[0][x]=V_BGR[1][x]=V_BGR[2][x]=0;//0xff;

		//原始檔 轉換後圖檔  RGB轉YCrCb
		cvCvtColor(ImgTmpBGR, ImgTmpYUV, CV_BGR2YCrCb);

		int Point[50]= {};

						for(int x=V_Start; x<=V_End; x++)
						{
							int nPixel=0, value=140;
							uchar Max_Gray_Value=0, Min_Gray_Value=255;
							for(int y=H_Start; y<=H_End; y++)
							{
								uchar TmpYUV[3]={}, TmpBGR[3]={};
								for(int c=0; c<ImgTmpBGR->nChannels; c++)
								{
									TmpBGR[c] = (unsigned char)ImgTmpBGR->imageData[y*ImgTmpBGR->widthStep + x*ImgTmpBGR->nChannels + c];
									TmpYUV[c] = (unsigned char)ImgTmpYUV->imageData[y*ImgTmpYUV->widthStep + x*ImgTmpYUV->nChannels + c];
								}

								if( TmpYUV[0]>Max_Gray_Value ) Max_Gray_Value=TmpYUV[0];
		                        if( TmpYUV[0]<Min_Gray_Value ) Min_Gray_Value=TmpYUV[0];
							}

		                    for(int y=H_Start; y<=H_End; y++)
		                    {
		                        uchar TmpBGR[3]={0};
		                        uchar TmpYUV = (unsigned char)ImgTmpYUV->imageData[y*ImgTmpYUV->widthStep + x*ImgTmpYUV->nChannels + 0];
		                        uchar TmpYUV_Up = (unsigned char)ImgTmpYUV->imageData[(y-1)*ImgTmpYUV->widthStep + x*ImgTmpYUV->nChannels + 0];
		                        uchar TmpYUV_Down = (unsigned char)ImgTmpYUV->imageData[(y+1)*ImgTmpYUV->widthStep + x*ImgTmpYUV->nChannels + 0];
		                        uchar thrDarkGray=thrDarkGray_value_y(ImgTmpYUV, x-1, x+1, H_Start, H_End);

		                        for(int c=0; c<ImgTmpBGR->nChannels; c++)
		                            TmpBGR[c] = (unsigned char)ImgTmpBGR->imageData[y*ImgTmpBGR->widthStep + x*ImgTmpBGR->nChannels + c];

		                        int value=165;
								if( Min_Gray_Value < 100 &&
		                            !( TmpBGR[0]>value || TmpBGR[1]>value || TmpBGR[2]>value ) &&
		                            (float)(TmpYUV-Min_Gray_Value) / (float)(Max_Gray_Value-Min_Gray_Value) < 0.5 &&
		                            TmpYUV < thrDarkGray
		                          )
		                        {
		                            for( int c=0; c<EndImage2->nChannels; c++ )
		                            {
		                                EndImage2->imageData[y*EndImage2->widthStep + x*EndImage2->nChannels + c ]= (unsigned char)0xff;
		                            }
		                        }
		                    }
						}

						//--- pixel_link

										int tmpdata[IMG_SIZE_X][IMG_SIZE_Y]={}, label=1;
										for(int x=V_Start; x<=V_End; x++)
											for(int y=H_Start; y<=H_End; y++)
												tmpdata[x][y]=(unsigned char)EndImage2->imageData[y*EndImage2->widthStep + x*EndImage2->nChannels + 0];

										for(int x=V_End; x>=V_Start; x--)  // 從右到左
										{
											for(int y=H_End; y>=H_Start; y--)
											{
												if(tmpdata[x][y] == 0xff)
												{
													pixel_link(tmpdata, V_Start, V_End, H_Start, H_End, x, y, label);

													int num=0;       //判斷雜訊
													for(int x1=V_Start; x1<=V_End; x1++)
														for(int y1=H_Start; y1<=H_End; y1++)
															if( tmpdata[x1][y1]==label )
																num++;
													if( num<= 12 )//(H_End-H_Start)/2 )
													{
														for(int x1=V_Start; x1<=V_End; x1++)
															for(int y1=H_Start; y1<=H_End; y1++)
																if( tmpdata[x1][y1]==label )
																	tmpdata[x1][y1]=0;
														label--;
													}
													//-----------
													label++;
												}
											}
										}
										for(int x=V_End; x>=V_Start; x--)
														{
															int num=0;
															for(int y=H_End; y>=H_Start; y--)
															{
																if( tmpdata[x][y]>=1 && tmpdata[x][y]<=20)
																//if( tmpdata[x][y]==2)  測試第n個
																{
											    					num++;
																	V_BGR[0][x]++;
											    					for(int c=0; c<EndImage2->nChannels; c++)
										    							EndImage2->imageData[y*EndImage2->widthStep + x*EndImage2->nChannels + c]=(uchar)0xff;
																}
															}
														}
										                //
										                //--- pixel_link

										int TmpPoint=0;
														for(int x=V_End; x>=V_Start; x--)
														{
															//printf("(%d)%d ",x, V_BGR[0][x]);
															if( (V_BGR[0][x]-V_BGR[0][x-1]>0) &&
																(V_BGR[0][x-1]==0) &&
																(V_BGR[0][x+1]>0)
																){
																	Point[TmpPoint] = x-1;
																	TmpPoint++;
															}

															if( (V_BGR[0][x]-V_BGR[0][x+1]>0) &&
																(V_BGR[0][x+1]==0 &&
																(V_BGR[0][x-1]>0))

																){
																	Point[TmpPoint] = x+1;
																	TmpPoint++;
															}
															if(TmpPoint>16)
																break;
														}



														int Grid_Point[8][4]={};
																		int number1=0;
																		for( int i=0, number=1; i<=TmpPoint-1; i++, number++)
																		{
																			if( number>0 )
																			{
																				if( i%2==1 )
																				{
																					Grid_Point[number1][0] = Point[i];        //x左
																					Grid_Point[number1][1] = Point[i-1];      //x右
																					Grid_Point[number1][2] = HPoint[0];       //y上
																					Grid_Point[number1][3] = HPoint[H_tmp-1]; //y下
																					number1++;

																					//printf("%d %d %d %d\n",Grid_Point[number1][0],Grid_Point[number1][1],Grid_Point[number1][2],Grid_Point[number1][3]);
																				}
																			}
																		}

																		//框架切割後顯示數字
																		//--縮小框架
																		for(int n=0; n<8; n++)
																		{
																		    int x_left=319, x_right=0, y_top=239, y_bottom=0;
														                    for(int x=Grid_Point[n][0]; x<=Grid_Point[n][1]; x++)
														                    {
														                        for(int y=Grid_Point[n][2]; y<=Grid_Point[n][3]; y++)
														                        {
														                            if( (unsigned char)EndImage2->imageData[y*EndImage2->widthStep+x*EndImage2->nChannels+0] == 0xff )
														                            {
														                                if(x<x_left)   x_left=x;
														                                if(x>x_right)  x_right=x;
																						if(y<y_top)    y_top=y;
																						if(y>y_bottom) y_bottom=y;
														                            }
														                        }
														                    }
														                    Grid_Point[n][0]=x_left;
														                    Grid_Point[n][1]=x_right;
														                    Grid_Point[n][2]=y_top;
														                    Grid_Point[n][3]=y_bottom;
																		}

																		for(int n=0; n<8; n++)
																		{
																			for(int x=Grid_Point[n][0]; x<=Grid_Point[n][1]; x++)
																			{
																				for(int y=Grid_Point[n][2]; y<=Grid_Point[n][3]; y++)
																				{
																					for(int c=0; c<EndImage1->nChannels; c++)
																						EndImage1->imageData[y*EndImage1->widthStep + x*EndImage1->nChannels+c] = EndImage2->imageData[y*EndImage2->widthStep + x*EndImage2->nChannels+c];
																				}
																			}
																		}

		/*  數字辨識  */
		//發票中的8個數字 切割
		//number
		int *digit=new int[8];
		for(int i=0; i<8; i++)
			digit[i]=1;
		int number2=1;
		//IplImage* tmp;
		//IplImage* tmp1;
		for(int n=0; n<8; n++)
		{
		    CvMat* mat_roi;
		    if( Grid_Point[n][0]>0 && Grid_Point[n][1]>0 &&
		        Grid_Point[n][2]>0 && Grid_Point[n][3]>0 )
		    {
		        IplImage* tmp = cvCreateImage(cvSize(25,25), IPL_DEPTH_8U, 3);
		        IplImage* tmp1 = cvCreateImage(cvSize((Grid_Point[n][1]-Grid_Point[n][0]+1),
		            (Grid_Point[n][3]-Grid_Point[n][2]+1)),
		            IPL_DEPTH_8U,3);

		        mat_roi = cvCreateMat((Grid_Point[n][3]-Grid_Point[n][2]+1),
		                              (Grid_Point[n][1]-Grid_Point[n][0]+1),CV_8UC3);

		        				//cvRect(X, Y, 寬, 高)
		        CvRect rect_roi = cvRect(Grid_Point[n][0], Grid_Point[n][2], (Grid_Point[n][1]-Grid_Point[n][0]+1),
		                                                        (Grid_Point[n][3]-Grid_Point[n][2]+1));
		        //從SRC影像中切割rect_roi位置和大小儲存在mat_roi陣列
		        cvGetSubRect(EndImage1, mat_roi, rect_roi);
		        //cvGetImage(陣列mat, 影像image)，將陣列轉成影像
		        cvGetImage(mat_roi, tmp1);
		        cvResize(tmp1, tmp, CV_INTER_LINEAR);    //縮放來源影像到目標影像

		        number2++;

		        //┐δ├╤
		        //-------
		        //辨識
		        						//-------
		        						int num[25]={0}, tot=0;
		        						int tot_sum[4]={0}, tot_min1=100, tot_min2=100, tot_average=0  ;
		                                int tot_top=0, tot_bottom=0, tot_right=0, tot_left=0, tot_center=0;

		        						//IplImage* img, int* n, int i, int x_start, int x_end, int y_start, int y_end
		        						White_Area(tmp, num, 0, 0			  , tmp->width/5,   0, tmp->height/5);
		        						White_Area(tmp, num, 1, tmp->width/5  , tmp->width*2/5, 0, tmp->height/5);
		        						White_Area(tmp, num, 2, tmp->width*2/5, tmp->width*3/5,	0, tmp->height/5);
		        						White_Area(tmp, num, 3, tmp->width*3/5, tmp->width*4/5, 0, tmp->height/5);
		        						White_Area(tmp, num, 4, tmp->width*4/5, tmp->width    , 0, tmp->height/5);

		        						White_Area(tmp, num, 5, 0			  , tmp->width/5,   tmp->height/5, tmp->height*2/5);
		        						White_Area(tmp, num, 6, tmp->width/5  , tmp->width*2/5, tmp->height/5, tmp->height*2/5);
		        						White_Area(tmp, num, 7, tmp->width*2/5, tmp->width*3/5,	tmp->height/5, tmp->height*2/5);
		        						White_Area(tmp, num, 8, tmp->width*3/5, tmp->width*4/5, tmp->height/5, tmp->height*2/5);
		        						White_Area(tmp, num, 9, tmp->width*4/5, tmp->width    , tmp->height/5, tmp->height*2/5);

		        						White_Area(tmp, num, 10, 0			   , tmp->width/5  , tmp->height*2/5, tmp->height*3/5);
		        						White_Area(tmp, num, 11, tmp->width/5  , tmp->width*2/5, tmp->height*2/5, tmp->height*3/5);
		        						White_Area(tmp, num, 12, tmp->width*2/5, tmp->width*3/5, tmp->height*2/5, tmp->height*3/5);
		        						White_Area(tmp, num, 13, tmp->width*3/5, tmp->width*4/5, tmp->height*2/5, tmp->height*3/5);
		        						White_Area(tmp, num, 14, tmp->width*4/5, tmp->width    , tmp->height*2/5, tmp->height*3/5);

		        						White_Area(tmp, num, 15, 0			   , tmp->width/5  , tmp->height*3/5, tmp->height*4/5);
		        						White_Area(tmp, num, 16, tmp->width/5  , tmp->width*2/5, tmp->height*3/5, tmp->height*4/5);
		        						White_Area(tmp, num, 17, tmp->width*2/5, tmp->width*3/5, tmp->height*3/5, tmp->height*4/5);
		        						White_Area(tmp, num, 18, tmp->width*3/5, tmp->width*4/5, tmp->height*3/5, tmp->height*4/5);
		        						White_Area(tmp, num, 19, tmp->width*4/5, tmp->width    , tmp->height*3/5, tmp->height*4/5);

		        						White_Area(tmp, num, 20, 0			   , tmp->width/5  , tmp->height*4/5, tmp->height);
		        						White_Area(tmp, num, 21, tmp->width/5  , tmp->width*2/5, tmp->height*4/5, tmp->height);
		        						White_Area(tmp, num, 22, tmp->width*2/5, tmp->width*3/5, tmp->height*4/5, tmp->height);
		        						White_Area(tmp, num, 23, tmp->width*3/5, tmp->width*4/5, tmp->height*4/5, tmp->height);
		        						White_Area(tmp, num, 24, tmp->width*4/5, tmp->width    , tmp->height*4/5, tmp->height);



		        						//----是否封閉

		        												int* Close_X=new int[tmp->height];  //open:1 close:2
		        												int* Close_Y=new int[tmp->width];  //open:1 close:2

		        												int Grid_Width=(Grid_Point[n][1]-Grid_Point[n][0]+1);
		        												int Grid_Height=(Grid_Point[n][3]-Grid_Point[n][2]+1);

		        												for (int i=0; i<tmp->height; i++)
		        													Close_X[i]=0;
		        						                        for (int i=0; i<tmp->width; i++)
		        													Close_Y[i]=0;

		        												int Close_number=0;
		        												for(int y=0; y<tmp->height; y++)
		        												{
		        													for(int x=0; x<tmp->width; x++)
		        													{
		        														int BGR=(uchar)tmp->imageData[ y*tmp->widthStep+x*tmp->nChannels ];
		        														Close_number++;

		        														if( BGR==0xff)
		        														{
		        															if( Close_number==1 )
		        																Close_X[y]++;
		        														}
		        														else
		        															Close_number=0;
		        													}

		        													Close_number=0;
		        												}
		        												printf("\n\n");

		        												Close_number=0;
		        												for(int x=0; x<tmp->width; x++)
		        												{
		        													for(int y=0; y<tmp->height; y++)
		        													{
		        														int BGR=(uchar)tmp->imageData[ y*tmp->widthStep+x*tmp->nChannels ];
		        														Close_number++;

		        														if( BGR==0xff)
		        														{
		        															if( Close_number==1 )
		        																Close_Y[x]++;
		        														}
		        														else
		        															Close_number=0;
		        													}
		        													//cout << endl;

		        													Close_number=0;
		        												}


		        												int rank_1[4]={0,0,0,0};  //x:1 1:上或下半部 2:下上下全部
		        												int rank_2[5]={0,0,0,0,0};  //x:2 1:上或下半部 2:下上下全部
		        												//-----------
		        												int total_x2_1=0; //Close_X:2  上
		        												int total_x2_2=0; //Close_X:2  下
		        												int total1=0, total2=0, rank_number=0;
		        												for (int i=0; i<tmp->height; i++)  //x:2
		        												{
		        													if( Close_X[i]>=2 )
		        													{
		        														total2++;
		        														total_x2_1++;
		        														if( total_x2_1>=15 )
		        															rank_2[4]=1;  //0
		        													}
		        													else
		        													{
		        														total1++;
		        														total_x2_1=0;
		        													}
		        												}

		        												int rank_number_1=0, rank_number_2=0;
		        												if( total2>=7 )
		        												{
		        													total_x2_1=1; //Close_X:2  上
		        													for (int i=0; i<tmp->height; i++)  //x:2
		        													{
		        														if( Close_X[i]>=2 && Close_X[i-1]>=2 && i>0 &&
		        															rank_number_1 < (int)(tmp->height/2)
		        														  )
		        														{
		        															if( rank_number_1==0 ) rank_number_1=i-1;
		        															total_x2_1++;

		        															if(	total_x2_1>=7 && rank_number_1 < (int)(tmp->height/2) )
		        																rank_2[0]=1;
		        														}
		        														else
		        														{
		        															total_x2_1=1;
		        															rank_number_1=0;
		        														}
		        													}

		        													//-----
		        													total_x2_2=1; //Close_X:2  下
		        													for (int i=0; i<tmp->height; i++)  //x:2
		        													{
		        														if( Close_X[i]>=2 && Close_X[i-1]>=2 && i>0 //&&
		        															//rank_number_2 >= (int)(tmp->height/2)
		        															)
		        														{
		        															if( rank_number_2==0 && i-1>= (int)(tmp->height/2) ) rank_number_2=i-1;
		        															total_x2_2++;

		        															if(	total_x2_2>=5 && rank_number_2 >= (int)(tmp->height/2) &&
		        																rank_2[0]==0
		        															  )
		        																rank_2[1]=1;
		        														}
		        														else
		        														{
		        															total_x2_2=1;
		        															rank_number_2=0;
		        														}
		        													}

		        													//---
		        													if( rank_2[0] >0 || rank_2[1]>0 )
		        													{

		        														total_x2_1=1; //Close_X:2  上
		        														total_x2_2=1; //Close_X:2  下
		        														int TF1=0, TF2=0;
		        														if( total2>=10 )
		        														{
		        															for (int i=0; i<tmp->height; i++)  //x:3
		        															{
		        																if( Close_X[i]>=2 && Close_X[i-1]>=2 && i>0 &&
		        																	rank_number_1 < (int)(tmp->height/2) && Close_X[12]==1
		        																	)
		        																{
		        																	if( rank_number_1==0 ) rank_number_1=i-1;
		        																	total_x2_1++;

		        																	if(	total_x2_1>=4 && rank_number_1 < (int)(tmp->height/2) )
		        																		TF1=1;
		        																}
		        																else
		        																{
		        																	total_x2_1=1;
		        																	rank_number_1=0;
		        																}
		        															}
		        															for (int i=0; i<tmp->height; i++)  //x:2
		        															{
		        																if( Close_X[i]>=2 && Close_X[i-1]>=2 && i>0
		        																	)
		        																{
		        																	if( rank_number_2==0 && i-1>= (int)(tmp->height/2) ) rank_number_2=i-1;
		        																	total_x2_2++;

		        																	if(	total_x2_2>=4 && rank_number_2 >= (int)(tmp->height/2) //&&
		        																		//rank_2[0]==0
		        																		)
		        																		TF2=1;
		        																}
		        																else
		        																{
		        																	total_x2_2=1;
		        																	rank_number_2=0;
		        																}
		        															}
		        															if( TF1==1 && TF2==1)
		        																rank_2[4]=2;
		        														}

		        													}
		        												}


		        												int total_x1=0; //Close_X:1
		        												total1=0, total2=0, rank_number=0;
		        												for (int i=0; i<tmp->height; i++)  //x:1
		        												{
		        													if( Close_X[i]==1 )
		        													{
		        														total_x1++;
		        														if( total_x1>=20 )
		        															rank_1[0]=1;  //number 1
		        													}

		        												}

		        												int total_y1=0; //Close_Y:1
		        												int total_y2=0; //Close_Y:2
		        												total1=0, total2=0, rank_number=0;
		        												for (int i=0; i<tmp->width; i++)  //Y:3
		        												{
		        													if( Close_Y[i]>=3 )
		        													{
		        														total_y2++;
		        														total2++;
		        														if( rank_number==0 ) rank_number=i;
		        														if( total_y2>=10 )
		        														{
		        															rank_1[2]=1;  //0
		        														}
		        													}
		        													else if( Close_Y[i]>=1 )
		        													{
		        														total_y1++;
		        														if( total_y1>=20 ) rank_1[2]=1;  //number 1
		        													}
		        													else
		        													{
		        														total1++;
		        														total_y2=0;
		        														rank_number=0;
		        													}
		        												}

		        												for(int j=0; j<25; j++)  tot+=num[j]; //總白點數

		        												tot_sum[0] = (num[0]+num[1]+num[5]+num[6]);
		        												tot_sum[1] = (num[3]+num[4]+num[8]+num[9]);
		        												tot_sum[2] = (num[15]+num[16]+num[20]+num[21]);
		        												tot_sum[3] = (num[18]+num[19]+num[23]+num[24]);

		        												tot_top    = (num[0]+num[1]+num[2]+num[3]+num[4]+num[5]+num[6]+num[7]+num[8]+num[9]);
		        												tot_bottom = (num[15]+num[16]+num[17]+num[18]+num[19]+num[20]+num[21]+num[22]+num[23]+num[24]);
		        												tot_right  = (num[3]+num[4]+num[8]+num[9]+num[13]+num[14]+num[18]+num[19]+num[23]+num[24]);
		        												tot_left   = (num[0]+num[1]+num[5]+num[6]+num[10]+num[11]+num[15]+num[16]+num[20]+num[21]);
		        						                        tot_center = (num[2]+num[7]+num[12]+num[17]+num[22]);

		        												for(int i=0; i<=3; i++)
		        													if( tot_min1 > tot_sum[i] )
		        														tot_min1 = tot_sum[i];

		        												for(int i=0; i<=3; i++)
		        													if( tot_min2 > tot_sum[i] && tot_sum[i]!=tot_min1  )
		        														tot_min2 = tot_sum[i];

		        												if( tot< 25*25 && tot>0 )
		        												{
		        													if( (num[0]+num[5]+num[1])<=24 && num[15]>0 //4 ...
		        														)
		        													{
		        														digit[n]=4;

		        													}

		        													else if( rank_2[0]>0 || rank_2[1]>0 )
		        						                            {
		        						                                if( rank_2[4]==1 )  //0
		        						                                {
		        						                                    digit[n]=0;
		        						                                }
		        														else if( rank_2[0]==0 && rank_2[1]==1 &&  //6...
		        																 num[7]+num[8]+num[9]<30 &&
		        																 num[9]<15 && num[15]>9
		        															   )
		        						                                {
		        						                                    digit[n]=6;
		        						                                }
		        						                                else if( rank_2[0]==1 && rank_2[1]==0 &&  //9
		        																 num[15]+num[16]+num[17]<24 && num[15]<13
		        															   )
		        						                                {
		        						                                    digit[n]=9;
		        						                                }
		        														else if( rank_2[4]==2 && num[10]>0  //8	...
		        															)
		        														{
		        															digit[n]=8;
		        														}
		        														else if( num[6]+num[10]+num[16]<24 &&  //3...
		        															     num[9]>0
		        															)
		        														{
		        															digit[n]=3;
		        														}
		        														else if( num[8]+num[9]<30 &&
		        															num[5]> 12 //5 ...
		        															)
		        														{
		        															digit[n]=5;
		        														}
		        						                            }
		        													else //if( rank_2[1]==0 )
		        													{
		        														if(	Grid_Width <= (int)5*(Grid_Height)/15  //1  ...
		        														  )
		        														{
		        															digit[n]=1;
		        														}
		        														else if( //rank_1[0]==1 &&
		        																 num[15]<=5 && num[20]<=5 && num[19]<=5 && num[24]<=5 //7 ...
		        															   )
		        														{
		        															digit[n]=7;
		        														}
		        														else if( num[18]<=15 && num[19]<=5 && num[11]<=13 &&  //2...
		        															((Close_Y[0]>=2 && Close_Y[1]>=2) ||
		        															(Close_Y[23]>=2 && Close_Y[24]>=2))
		        															)
		        														{
		        															digit[n]=2;
		        														}
		        														else if( num[6]+num[10]+num[16]<20 &&  //3...
		        																 num[9]>0
		        															)
		        														{
		        															digit[n]=3;
		        														}
		        														else if( num[8]+num[9]<30 &&
		        															     num[5]> 12 //5 ...
		        															)
		        														{
		        															digit[n]=5;
		        														}

		        						                            }


		        												}

		        						                    }
		        						                }







		        //cout << endl;
		        //cvReleaseImage(&tmp);
		        //cvReleaseImage(&tmp1);
		        //cvReleaseMat(&mat_roi);

		    }
		    //-------
		}

		//-----------------
		char s1[30];
		sprintf(s1,"%d%d%d%d%d%d%d%d", digit[7],digit[6],digit[5],digit[4],digit[3],digit[2],digit[1],digit[0]);
		CvFont Font1=cvFont(2,1);
		cvRectangle(EndImage1, cvPoint(0,10), cvPoint(319,50), CV_RGB(0,0,0),-1, 8, 0 );
		cvPutText(EndImage1,s1,cvPoint(50,40),&Font1,CV_RGB(0,255,255));
	}

	//畫格線-白框
	Grid(EndImage1, Grid_X1, Grid_X2, Grid_Y1, Grid_Y2, CV_RGB(255,255,255));
	//畫格線-藍框
	Grid(EndImage1, VPoint[0], VPoint[V_tmp-1], HPoint[0], HPoint[H_tmp-1], CV_RGB(255,255,0));

	//---------------------------
	//cvResize(ImgTmpYUV, frame);
	cvResize(EndImage1, frame);
	cvReleaseImage(&EndImage1);
	cvReleaseImage(&EndImage2);
	cvReleaseImage(&ImgTmpYUV);
	cvReleaseImage(&ImgTmpBGR);

	return ChangeAlert;

}


} // end-extern-C

