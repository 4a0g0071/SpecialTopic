// ex01.主控台應用程式的進入點。
//
#include "stdafx.h"
#include <highgui.h>
#include <cv.h>
#include <iostream>
#include <fstream>
#include <string>
#include <queue>

using namespace std;
//--------------------------------------
#define select1 0   //0:不列印比重值、封閉數值	1:列印封閉數值
#define select2 0   //0:不判斷翻頁			1:判斷翻頁
#define select3 0   //0:不儲存數字圖			1:儲存8個數字圖	2:儲存單一數字圖
#define select5 0   //0:不顯示切割線			1:顯示切割線
#define select6 0   //0:影像檔				1:WebCam
//--------------------------------------
#define IMG_SIZE_X (320)  //原始檔寬
#define IMG_SIZE_Y (240)  //原始檔長
#define sw (1+1)          //silding windows
#define	COLOR_FG	'1'
#define	COLOR_BG	'0'  //Black Point

//camera
//data
unsigned long V_BGR[3][IMG_SIZE_X];  //Vertical  :垂直
unsigned long H_BGR[3][IMG_SIZE_Y];  //Horizontal:水平
int Grid_X1=60;      //X軸格線起點  //70
int Grid_X2=220;     //X軸格線終點  //210
int Grid_Y1=80;      //Y軸格線起點  //130
int Grid_Y2=200;     //Y軸格線終點  //200

int VPoint[100], V_tmp=0;
int HPoint[100], H_tmp=0;
//暫存檔
int nRow=0,nCol=0;
int SmoothOffset_V=1;         //-n..點..+n 累加取平均 Smooth:平滑 Vertical
int SmoothOffset_H=1;         //-n..點..+n 累加取平均 Smooth:平滑 Horizontal
int thrDarkGray;
IplImage* ImgTmpBGR;
IplImage* ImgTmpYUV;

//UI
IplImage* BackImage=cvCreateImage(cvSize(960,480),IPL_DEPTH_8U,3);  //全部圖整合在一起
IplImage* frame;                                                    //擷取圖 左上
IplImage* Image;                                                    //折線圖 左下
IplImage* EndImage;                                                 //修改後圖 右下
IplImage* EndImage1;
IplImage* EndImage2;
IplImage* OriImage;
IplImage* two_one_Image; //=cvCreateImage(cvSize(140,100),IPL_DEPTH_8U,3);

IplImage* tmp;   //縮放二值化圖數字大小
IplImage* tmp1;
CvMat* mat_roi;
IplImage* tmp2;  //縮放原始圖數字大小
IplImage* tmp3;
CvMat* mat_roi1;
IplImage* tmp4;
//---------------------------------------
long* Array_KsumISub;
int* Array_ISubTemp;
//---------------------------------------
inline unsigned long min(unsigned long a, unsigned long b)
{
	if( a<b ) return a;
	else      return b;
}
void ProcImgCopy(IplImage*, IplImage* , int, int); //多重圖整合一張圖
void Grid(IplImage*, unsigned long, unsigned long, unsigned long, unsigned long, CvScalar);  //畫格線
void Smooth(int, int, int);  //平滑化:起點  寬度  圖片size
int thrDarkGray_value_x(IplImage*, int, int, int, int); //判斷門檻值
int thrDarkGray_value_y(IplImage*, int, int, int, int); //判斷門檻值
int ImgChange(IplImage*, int); //是否翻頁
void Cut_Line(IplImage*, int, int, CvScalar); //切割線 縱向 橫向 顏色
void White_Area(IplImage* img, int*, int, int, int, int, int); //25個區域-白點數
void onMouse1(int Event,int x,int y,int flags,void* param);
void onMouse2(int Event,int x,int y,int flags,void* param);
void int2str(int, int, int , int , int, int, int, char *);
void int2str1(int, int, int , int , int, int, int, char *);
void pixel_link(int [IMG_SIZE_X][IMG_SIZE_Y], int, int, int, int, int, int, int);  //判斷亮點
void pixel_link_1(char** const ImgArray, int, int, int, int, int, int, int); //判斷暗點
void FreeArray1 (int*& ImgArray);  //記憶體釋放 一維陣列
void FreeArray2(char**& ImgArray, int n); //記憶體釋放 二維陣列
bool Init_Image(char**& ImgArray, int n); //初始化 二維陣列
void PrintImage (char** const ImgArray, long n);             //print
void IsRegion (queue<long>& queue_FIFO, char** const ImgArray,   
	const long row, const long col, const long length); //佇列 Region True or False Black Point
void Mark_Region (char** const ImgArray, const long ptr, const long length); //0 or 1


int main(int argc, char *argv[])
{
	CvCapture* capture;
	if( select6==0 )
		capture = cvCreateFileCapture("d:\\test.avi");  //avi
	else
		capture = cvCaptureFromCAM(0) ;                 //webcam

	CvPoint FromPoint1,ToPoint1;
	//frame = cvQueryFrame(capture);

	unsigned int number2[10]={0};
	char buffer[9999];

	Array_KsumISub=new long[10];
	Array_ISubTemp=new int[IMG_SIZE_X*IMG_SIZE_Y];//ISub

	for(int i=0;i<10;i++)
		Array_KsumISub[i]=0;

    //http://debut.cis.nctu.edu.tw/~ching/Course/C++/Handout/12%20File%20IO/File%20Access.htm
    //字串顯示
    //ofstream 檔案變數("檔名");   //將資料輸出到檔案
	/*
    ifstream inFile("digital.txt");  //讀入檔案中的資料
    string line;
    getline(inFile,line);
    inFile.close();
    string str[7];
    for(int n=0; n<5; n++)
        str[n]=line.substr((8*n),8);
    str[5]=line.substr(40,3);
    str[6]=line.substr(43,3);
    cout << str[0] << " " << str[1] << " " << str[2] << " " << str[3] << " " << str[4] << " " << str[5] << " " << str[6] << endl;
    //cout << str[2].substr(5,3) << " " << str[3].substr(5,3) << " " << str[4].substr(5,3) << " " << endl;
	*/

	int kkk=0;
	while(true)
	{
		frame = cvQueryFrame(capture);

		if (!frame) break;

        int Img_Change=0;
		if( select2 == 1 )
		{
		    Img_Change=ImgChange(frame, kkk);   //翻頁
		    kkk++;
        }

		if(Img_Change==0)
		{
			//視窗右上方原始圖
			ProcImgCopy(frame, BackImage, 2*frame->width, 0); //右上

			if ( Image == NULL )
				Image   =cvCreateImage(cvSize(IMG_SIZE_X,IMG_SIZE_Y),IPL_DEPTH_8U,3);
			if ( EndImage == NULL )
				EndImage=cvCreateImage(cvSize(IMG_SIZE_X,IMG_SIZE_Y),IPL_DEPTH_8U,3);
			if ( EndImage1 == NULL )
				EndImage1=cvCreateImage(cvSize(IMG_SIZE_X,IMG_SIZE_Y),IPL_DEPTH_8U,3);
			if ( EndImage2 == NULL )
				EndImage2=cvCreateImage(cvSize(IMG_SIZE_X,IMG_SIZE_Y),IPL_DEPTH_8U,3);


			//複製一份完整的IplImage資料結構圖形及設定cvCloneImage(IplImage資料結構)
			ImgTmpBGR = cvCloneImage(frame);
			OriImage  = cvCloneImage(frame);

			//         原始檔 轉換後圖檔  RGB轉YCrCb
			cvCvtColor(frame, frame, CV_BGR2YCrCb);

			ImgTmpYUV = cvCloneImage(frame);

			//------------------------------
			unsigned long Tmp[3]= {};
			unsigned long nThrPixel = 0;
			const int position_offset_V = 4;
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
					for ( int c=0; c<ImgTmpYUV->nChannels; c++ ){
						TmpBGR[c] = (unsigned char)ImgTmpBGR->imageData[y*ImgTmpBGR->widthStep + x*ImgTmpBGR->nChannels + c];
						TmpYUV[c] = (unsigned char)ImgTmpYUV->imageData[y*ImgTmpYUV->widthStep + x*ImgTmpYUV->nChannels+c];
					}
					if( TmpYUV[0] < thrDarkGray*.6 )  //.6
						V_BGR[0][x] = min(V_BGR[0][x], ImgTmpYUV->imageData[ y*ImgTmpYUV->widthStep + x*ImgTmpYUV->nChannels ]);
				}
			}

			//平滑化 V     起點      圖片size     V
			Smooth(SmoothOffset_V, IMG_SIZE_X, 1);

			//畫折線圖:Vertical Blue
			unsigned long V_BGRMin = 255;
			cvSet(Image, cvScalar(0,0,0));
			for(int x=0; x<IMG_SIZE_X; x++)
				if( x>=(Grid_X1-position_offset_V*2) && x<(Grid_X2+position_offset_V*2) )
					if ( V_BGRMin > V_BGR[0][x] )
						V_BGRMin = V_BGR[0][x];

			for(int x=0; x<IMG_SIZE_X; x++)
			{
				FromPoint1 = cvPoint(x  ,(unsigned char)(V_BGR[0][x]));
				ToPoint1   = cvPoint(x+1,(unsigned char)(V_BGR[0][x+1]));
				cvLine(Image, FromPoint1, ToPoint1, CV_RGB(255,0,255), 1);
			}

			//動態選取範圍 V
			for( int i=0; i<100; i++)
				VPoint[i]=0;
			V_tmp=0;

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
							//break;
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
					//int value=thrDarkGray*.9;
					for ( int c=0; c<ImgTmpYUV->nChannels; c++ )
					{
						TmpBGR[c] = (unsigned char)ImgTmpBGR->imageData[y*ImgTmpBGR->widthStep + x*ImgTmpBGR->nChannels + c];
						TmpYUV[c] = (unsigned char)ImgTmpYUV->imageData[y*ImgTmpYUV->widthStep + x*ImgTmpYUV->nChannels + c];
					}
					if( TmpYUV[0] < (int)thrDarkGray*.8 &&
						TmpBGR[0]<(int)thrDarkGray*.9 && (int)TmpBGR[1]<thrDarkGray*.9 && TmpBGR[2]<(int)thrDarkGray*.9
						){
							//min
							H_BGR[0][y]= nNumDarkPixel++;
							for(int c=0; c<ImgTmpYUV->nChannels; c++)  //Color:255
								ImgTmpYUV->imageData[y*ImgTmpYUV->widthStep + x*ImgTmpYUV->nChannels + c] = (unsigned char)0xff;
					}
					else
						// Not dark gray
						for(int c=0; c<ImgTmpYUV->nChannels; c++)  //Color:0
							ImgTmpYUV->imageData[y*ImgTmpYUV->widthStep + x*ImgTmpYUV->nChannels + c] = (unsigned char)0;
				}
			}


			//平滑化 H  起點Y         圖片size  H
			Smooth(SmoothOffset_H, IMG_SIZE_Y, 1);

			//畫折線圖:Horizontal Blue
			for(int y=0; y<IMG_SIZE_Y; y++)
			{
				FromPoint1 = cvPoint((unsigned char)(H_BGR[0][y]),y);
				ToPoint1   = cvPoint((unsigned char)(H_BGR[0][y+1]),y+1);
				cvLine(Image,FromPoint1,ToPoint1,CV_RGB(255,255,0),1);
			}

			//動態選取範圍 H
			for(int i=0; i<100; i++)
				HPoint[i]= 0;
            H_tmp=0;

			for(int y=Grid_Y1+1; y-1<Grid_Y2; y++)
			{
				if ( ((long)H_BGR[0][y] - (long)H_BGR[0][y-position_offset_H] > (int)thrDarkGray*.1 ) &&
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

			/*
			//畫格線-藍框
			CvScalar color=CV_RGB(0,255,255);
			if( VPoint[0]>0 && VPoint[V_tmp-1]>0 && HPoint[0]>0 && HPoint[H_tmp-1]>0 )
			{
				Grid(frame, VPoint[0], VPoint[V_tmp-1], HPoint[0], HPoint[H_tmp-1], color);
				Grid(Image, VPoint[0], VPoint[V_tmp-1], HPoint[0], HPoint[H_tmp-1], color);

			}
			*/

			//----------------------------------------------
			//再次處理影像
			//存成陣列 V_BGR: 0:Blue 1:Green 2:Red
			cvSet(EndImage, cvScalar(0,0,0));
			cvSet(EndImage1, cvScalar(0,0,0));
			cvSet(EndImage2, cvScalar(0,0,0));

			int V_Start=VPoint[0];
			int V_End  =VPoint[V_tmp-1];
			int H_Start=HPoint[0];
			int H_End  =HPoint[H_tmp-1];

			for(int x=0; x<IMG_SIZE_X; x++)
				V_BGR[0][x]=V_BGR[1][x]=V_BGR[2][x]=0;//0xff;

			//動態選取範圍>0
			if(VPoint[0]>0 && VPoint[V_tmp-1]>0 && HPoint[0]>0 && HPoint[H_tmp-1]>0)
			{
				V_Start=VPoint[0];
				V_End  =VPoint[V_tmp-1];
				H_Start=HPoint[0];
				H_End  =HPoint[H_tmp-1];

				//         原始檔 轉換後圖檔  RGB轉YCrCb
				cvCvtColor(ImgTmpBGR, ImgTmpYUV, CV_BGR2YCrCb);

				//二值化
				//
				int Point[50]= {};
				//unsigned char* Min_Gray_Value = new unsigned char[V_End-V_Start+1];
				//unsigned char* Max_Gray_Value = new unsigned char[V_End-V_Start+1];
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
					//printf("Max_Gray=%d Min_Gray=%d X=%d \n",Max_Gray_Value,Min_Gray_Value,x);

                    for(int y=H_Start; y<=H_End; y++)
                    {
                        uchar TmpBGR[3]={0};
                        //uchar TmpYUV = (unsigned char)ImgTmpYUV->imageData[y*ImgTmpYUV->widthStep + x*ImgTmpYUV->nChannels + 0];
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
                                //Image->imageData[y*Image->widthStep + x*Image->nChannels + c ]= (unsigned char)0xff;
                            }
                        }
                        /*
                        else if( Min_Gray_Value < 110 &&
                                 !( TmpBGR[0]>value || TmpBGR[1]>value || TmpBGR[2]>value ) &&
                                 (float)(TmpYUV-Min_Gray_Value) / (float)(Max_Gray_Value-Min_Gray_Value) >= 0.4 &&
                                 (float)(TmpYUV-Min_Gray_Value) / (float)(Max_Gray_Value-Min_Gray_Value) <= 0.5
                               )
                        {
                                if( ((float)(TmpYUV_Up-Min_Gray_Value) / (float)(Max_Gray_Value-Min_Gray_Value) >= 0.4 &&
                                    (float)(TmpYUV_Up-Min_Gray_Value) / (float)(Max_Gray_Value-Min_Gray_Value) <= 0.5) ||
                                    ((float)(TmpYUV_Down-Min_Gray_Value) / (float)(Max_Gray_Value-Min_Gray_Value) >= 0.4 &&
                                    (float)(TmpYUV_Down-Min_Gray_Value) / (float)(Max_Gray_Value-Min_Gray_Value) <= 0.5)
                                )
                                {
                                    EndImage2->imageData[y*EndImage2->widthStep + x*EndImage2->nChannels + 0 ]= (unsigned char)0x00;
                                    EndImage2->imageData[y*EndImage2->widthStep + x*EndImage2->nChannels + 1 ]= (unsigned char)0xff;
                                    EndImage2->imageData[y*EndImage2->widthStep + x*EndImage2->nChannels + 2 ]= (unsigned char)0xff;
                                }
                                //V_BGR[0][x]++;
                        }
                        */
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

				cvSet(EndImage2, cvScalar(0,0,0));
				//框架外				
				for(int x=0; x<Grid_X1; x++)
					for(int y=0; y<ImgTmpBGR->height; y++)
						for(int c=0; c<ImgTmpBGR->nChannels; c++)
							EndImage2->imageData[y*EndImage2->widthStep+x*EndImage2->nChannels+c]=ImgTmpBGR->imageData[y*ImgTmpBGR->widthStep+x*ImgTmpBGR->nChannels+c];

				for(int x=Grid_X2+1; x<ImgTmpBGR->width; x++)
					for(int y=0; y<ImgTmpBGR->height; y++)
						for(int c=0; c<ImgTmpBGR->nChannels; c++)
							EndImage2->imageData[y*EndImage2->widthStep+x*EndImage2->nChannels+c]=ImgTmpBGR->imageData[y*ImgTmpBGR->widthStep+x*ImgTmpBGR->nChannels+c];

				for(int y=0; y<Grid_Y1; y++)
					for(int x=0; x<ImgTmpBGR->width; x++)
						for(int c=0; c<ImgTmpBGR->nChannels; c++)
							EndImage2->imageData[y*EndImage2->widthStep+x*EndImage2->nChannels+c]=ImgTmpBGR->imageData[y*ImgTmpBGR->widthStep+x*ImgTmpBGR->nChannels+c];

				for(int y=Grid_Y2+1; y<ImgTmpBGR->height; y++)
					for(int x=0; x<ImgTmpBGR->width; x++)
						for(int c=0; c<ImgTmpBGR->nChannels; c++)
							EndImage2->imageData[y*EndImage2->widthStep+x*EndImage1->nChannels+c]=ImgTmpBGR->imageData[y*ImgTmpBGR->widthStep+x*ImgTmpBGR->nChannels+c];                
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
				//printf("\n\n");

				//框架切割後顯示數字
				//int Grid_Point[8][4]={}, number1=0;
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

				
				//---
				/*  數字辨識  */
                //發票中的8個數字 切割
                //number
                int *digit=new int[8];
                for(int i=0; i<8; i++)
                    digit[i]=1;

				if( select3 == 1 || select3 == 0)
					two_one_Image=cvCreateImage(cvSize(140,100),IPL_DEPTH_8U,3);
				else if( select3 == 2 )
					two_one_Image=cvCreateImage(cvSize(50,25),IPL_DEPTH_8U,3);

				for(int n=0; n<8; n++)
				{
                    if( Grid_Point[n][0]>0 && Grid_Point[n][1]>0 &&
                        Grid_Point[n][2]>0 && Grid_Point[n][3]>0 )
                    {
						//辨識數字
						tmp = cvCreateImage(cvSize(25,25), IPL_DEPTH_8U, 3);
						tmp1 = cvCreateImage(cvSize((Grid_Point[n][1]-Grid_Point[n][0]+1),
                                                    (Grid_Point[n][3]-Grid_Point[n][2]+1)),
                                                    IPL_DEPTH_8U,3);
													//高              寬
						mat_roi = cvCreateMat((Grid_Point[n][3]-Grid_Point[n][2]+1),
                                              (Grid_Point[n][1]-Grid_Point[n][0]+1),CV_8UC3);
										//cvRect(X, Y, 寬, 高)
						CvRect rect_roi = cvRect(Grid_Point[n][0], Grid_Point[n][2], (Grid_Point[n][1]-Grid_Point[n][0]+1),
												(Grid_Point[n][3]-Grid_Point[n][2]+1));
						//從SRC影像中切割rect_roi位置和大小儲存在mat_roi陣列
						cvGetSubRect(EndImage1, mat_roi, rect_roi);
						//cvGetImage(陣列mat, 影像image)，將陣列轉成影像
						cvGetImage(mat_roi, tmp1);
						cvReleaseMat(&mat_roi);
						cvResize(tmp1, tmp, CV_INTER_NN);    //縮放來源影像到目標影像

						//原始圖
						tmp2 = cvCreateImage(cvSize(25,25), IPL_DEPTH_8U, 3);
						tmp3 = cvCreateImage(cvSize((Grid_Point[n][1]-Grid_Point[n][0]+1),
                                                    (Grid_Point[n][3]-Grid_Point[n][2]+1)),
                                                    IPL_DEPTH_8U,3);
													//高              寬
						mat_roi1 = cvCreateMat((Grid_Point[n][3]-Grid_Point[n][2]+1),
                                              (Grid_Point[n][1]-Grid_Point[n][0]+1),CV_8UC3);
										//cvRect(X, Y, 寬, 高)
						CvRect rect_roi1 = cvRect(Grid_Point[n][0], Grid_Point[n][2], (Grid_Point[n][1]-Grid_Point[n][0]+1),
												(Grid_Point[n][3]-Grid_Point[n][2]+1));
						//從SRC影像中切割rect_roi位置和大小儲存在mat_roi陣列
						cvGetSubRect(OriImage, mat_roi1, rect_roi1);
						//cvGetImage(陣列mat, 影像image)，將陣列轉成影像
						cvGetImage(mat_roi1, tmp3);
						cvReleaseMat(&mat_roi1);
						cvResize(tmp3, tmp2, CV_INTER_NN);    //縮放來源影像到目標影像
						
						//----是否封閉
						//-判斷Black number----
						int rank[4]={0,0,0,0}; // [0]=上(9) [1]=下(6) [2]=2個(8)  [0][1]:1=0數字 [0][1]:0=4數字
																							
						char** ImgArray = NULL;							
						if( tmp->width<0 || tmp->height<0 ) break;
						if ( !Init_Image(ImgArray, 25) ) break;										

						for(int row=0; row<25; row++)
						{
							for(int col=0; col<25; col++)
							{
								if( (uchar)tmp->imageData[row*tmp->widthStep + col*tmp->nChannels+0] == (uchar)0xff )
									ImgArray[row][col]=(uchar)99;//COLOR_FG;
								else
									ImgArray[row][col]=(uchar)0;//COLOR_BG;								
							}							
						}
						//PrintImage(ImgArray, 25);
					
						/*
						int number_Region = 0;
						for(int row=0; row<25; row++)
						{
							for(int col=0; col<25; col++)
							{
								if ( ImgArray[row][col] == COLOR_BG )
								{
									number_Region++;									
									Mark_Region(ImgArray, (row*25 + col), 25 );

									//cout << number_Region << endl;
									//PrintImage(ImgArray, 25);
								}
							}
						}
						cout << "(" << n << ")" << number_Region << endl << endl;
						*/
						
						label=1;
						for(int x=0; x<25; x++)  // 從左到右
						{
							for(int y=0; y<25; y++)
							{								
								if(ImgArray[x][y] == 0)//COLOR_BG)
								{
									pixel_link_1(ImgArray, 0, 25, 0, 25, x, y, label);
									label++;
								}								
							}
						}
						label--;
						if( select1==1)
						{							
							printf("(%d)%d \n",n,label);						
							PrintImage(ImgArray, 25);
						}

						for(int i=1, number=0; i<=label; i++)
						{
							int x_left=24, x_right=0, y_top=24, y_bottom=0;
							for(int x=0; x<25; x++)
							{
								for(int y=0; y<25; y++)
								{
									if( (unsigned char)ImgArray[x][y] == i )
									{
										if(y<x_left)   x_left=y;
										if(y>x_right)  x_right=y;
										if(x<y_top)    y_top=x;
										if(x>y_bottom) y_bottom=x;												
									}
								}
							}

							if( !(x_left==0 || x_right==24 || y_top==0 || y_bottom==24) )
							{									
								if( y_top<7 && y_bottom<18 )	rank[0]=1;									
								if( y_top>10 && y_bottom<24 )   rank[1]=1;
								if( y_bottom-y_top+1>13)	
								{
									rank[0]=1;
									rank[1]=1;
								}									
								if( y_bottom-y_top+1>3 ) number++;									
								if( number==2 )	rank[2]=1;
								rank[3]=1;
								if( select1==1)
									printf("%2d %2d %2d %2d (上：%d 下：%d 8：%d 全：%d)\n",x_left,x_right,y_top,y_bottom,rank[0],rank[1], rank[2], rank[3]);
							}								
						}
						

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
						//-----------												
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

						if( select1 == 1)
						{
							printf("%d\n",8-n);
							printf( "%2d %2d %2d %2d %2d \n",num[0],num[1],num[2],num[3],num[4]);
							printf( "%2d %2d %2d %2d %2d \n",num[5],num[6],num[7],num[8],num[9]);
							printf( "%2d %2d %2d %2d %2d \n",num[10],num[11],num[12],num[13],num[14]);
							printf( "%2d %2d %2d %2d %2d \n",num[15],num[16],num[17],num[18],num[19]);
							printf( "%2d %2d %2d %2d %2d \n\n",num[20],num[21],num[22],num[23],num[24]);
							printf( "0:%2d 1:%2d 2:%2d 3:%2d  %2d %2d \n\n",tot_sum[0],tot_sum[1],tot_sum[2],tot_sum[3],tot_min1,tot_min2);
							//printf( "%2d %2d %2d  %2d %2d %2d  %2d %2d\n",tot_top,tot_bottom,1.0-tot_top-tot_bottom,tot_left,tot_right,1.0-tot_left-tot_right,tot_center,tot_bevel);
							cout << endl;
						}
						int Grid_Width=(Grid_Point[n][1]-Grid_Point[n][0]+1);
						int Grid_Height=(Grid_Point[n][3]-Grid_Point[n][2]+1);

						ProcImgCopy(tmp2, two_one_Image, tmp2->width, 0); //右 原始圖

                        if( select5 == 1)
							Cut_Line(tmp, 5, 5, CV_RGB(255,0,0));

                        ProcImgCopy(tmp, two_one_Image, 0, 0);  //左 二值化圖

						if( tot< 25*25 && tot>0 )
						{							
							if( rank[3]==1 )  //封閉
							{																					                            
								if( rank[0]==1 && rank[1]==1 && rank[2]==0 ) //0
								{
									digit[n]=0;
									if( select3 == 2)
									{
										number2[0]++;
										sprintf(buffer, "C:\\img\\0\\img%u.png", number2[0]);
										cvSaveImage(buffer,two_one_Image);
									}
								}                                																
								if( (rank[0]==0 && rank[1]==1 && rank[2]==0 && num[5]>5) //6...
									
									)
								{
									digit[n]=6;
									if( select3 == 2)
									{
									   number2[6]++;
									   sprintf(buffer, "C:\\img\\6\\img%u.png", number2[6]);
									   cvSaveImage(buffer,two_one_Image);
									}
								}								
								if( (rank[0]==1 && rank[1]==0 && rank[2]==0 && num[5]>12) //9
								  )
								{
									digit[n]=9;
									if( select3 == 2)
									{
										number2[9]++;
										sprintf(buffer, "C:\\img\\9\\img%u.png", number2[9]);
										cvSaveImage(buffer,two_one_Image);
									}
								}
								if( rank[2]==1  //8	...								
									)
								{
									digit[n]=8;
									if( select3 == 2)
									{
										number2[8]++;
										sprintf(buffer, "C:\\img\\8\\img%u.png", number2[8]);
										cvSaveImage(buffer,two_one_Image);
									}
								}								
								if( num[5]<=12 && rank[2]==0 //4									
									)													
								{
									digit[n]=4;
									if( select3 == 2)
									{
										number2[4]++;
										sprintf(buffer,"c:\\img\\4\\img%u.png", number2[4]);
										cvSaveImage(buffer,two_one_Image);
									}
								}
								if( num[5]>12 && num[15]>12 && num[9]>15 && num[14]>10  &&
									!(rank[0]==1 && rank[1]==1) ) //8									
								{
									digit[n]=8;
									if( select3 == 2)
									{
										number2[8]++;
										sprintf(buffer, "C:\\img\\8\\img%u.png", number2[8]);
										cvSaveImage(buffer,two_one_Image);
									}
								}

							}
							else  //沒封閉
							{
								if( (num[0]+num[5]+num[1])<=24 && num[15]>0 //4
									)													
								{
									digit[n]=4;
									if( select3 == 2)
									{
										number2[4]++;
										sprintf(buffer,"c:\\img\\4\\img%u.png", number2[4]);
										cvSaveImage(buffer,two_one_Image);
									}
								}
								if( num[6]+num[10]+num[16]<30 &&  //3...
									num[17]<10 && num[20]>0 && 
									num[9]>0 && num[19]>0
									)
								{
									digit[n]=3;
									if( select3 == 2)
									{
										number2[3]++;
										sprintf(buffer,"c:\\img\\3\\img%u.png", number2[3]);
										cvSaveImage(buffer,two_one_Image);
									}
								}
								if( num[18]<=15 && num[11]<=13 &&  //2...
									num[20]>0 && num[24]>0 &&
									num[9]>0 && num[19]<10
									)
								{
									digit[n]=2;
									if( select3 == 2)
									{
										number2[2]++;
										sprintf(buffer,"c:\\img\\2\\img%u.png", number2[2]);
										cvSaveImage(buffer,two_one_Image);
									}
								}
								if( num[8]+num[9]<30 && num[5]> 15 && //5 ...
									num[20]>0 && num[24]>0 &&
									num[19]>0
									)
								{
									digit[n]=5;
									if( select3 == 2)
									{
										number2[5]++;
										sprintf(buffer, "C:\\img\\5\\img%u.png", number2[5]);
										cvSaveImage(buffer,two_one_Image);
									}
								}
								if( num[7]<10 &&
									num[15]<=5 && num[20]<=5 && num[19]<=5 && num[24]<=5 //7 ...									
									)
								{
									digit[n]=7;
									if( select3 == 2)
									{
										number2[7]++;
										sprintf(buffer, "C:\\img\\7\\img%u.png", number2[7]);
										cvSaveImage(buffer,two_one_Image);
									}
								}								
								if(	Grid_Width <= (int)5*(Grid_Height)/15  //1  ...
									)
								{
									digit[n]=1;
									if( select3 == 2)
									{
										number2[1]++;
										sprintf(buffer, "C:\\img\\1\\img%u.png", number2[1]);
										cvSaveImage(buffer,two_one_Image);
									}
								}								
								
							}
							
						}

						if( select3 == 1 )
						{
							//白點
                            tmp4 = cvCreateImage(cvSize(140,75), IPL_DEPTH_8U, 3);
                            CvFont Font1=cvFont(1,1);
                            cvRectangle(tmp4, cvPoint(0,0), cvPoint(140,100), CV_RGB(0,0,0),-1, 8, 0 );

							for(int x=0; x<5; x++)
                            {
                                char s2[20];
                                sprintf(s2,"%2d %2d %2d %2d %2d", num[0+x*5],num[1+x*5],num[2+x*5],num[3+x*5],num[4+x*5]);
                                cvPutText(tmp4,s2,cvPoint(0,x*15+12),&Font1,CV_RGB(255,255,255));
                            }

							ProcImgCopy(tmp4, two_one_Image, 0, tmp2->height); //右

						    sprintf(buffer,"img%u.png", 8-n);  //8-n  n+1
							if( select5 == 1)
								Cut_Line(tmp, 5, 5, CV_RGB(255,0,0));
                            cvSaveImage(buffer,two_one_Image);
						}
                    }										
                }
				if( select1 == 1)
				{
					cout << endl;

					for(int n=7; n>=0; n--)
					{
						cout << digit[n];
					}
					cout << endl;
				}


                //辨識8個數字
				char s1[16];
                sprintf(s1,"%d%d%d%d%d%d%d%d", digit[7],digit[6],digit[5],digit[4],digit[3],digit[2],digit[1],digit[0]);
                CvFont Font1=cvFont(2,1);
                cvRectangle(EndImage2, cvPoint(0,10), cvPoint(319,50), CV_RGB(0,0,0),-1, 8, 0 );

                /*
				char d1[2], d2[2], d3[2], d4[2], d5[2], d6[2], d7[2], d8[2];
                sprintf(d1,"%d",digit[7]);
                sprintf(d2,"%d",digit[6]);
                sprintf(d3,"%d",digit[5]);
                sprintf(d4,"%d",digit[4]);
                sprintf(d5,"%d",digit[3]);
                sprintf(d6,"%d",digit[2]);
                sprintf(d7,"%d",digit[1]);
                sprintf(d8,"%d",digit[0]);

                if( (d6==str[2].substr(5,1) && d7==str[2].substr(6,1) && d8==str[2].substr(7,1)) ||
                    (d6==str[3].substr(5,1) && d7==str[3].substr(6,1) && d8==str[3].substr(7,1)) ||
                    (d6==str[4].substr(5,1) && d7==str[4].substr(6,1) && d8==str[4].substr(7,1)) ||
                    (d6==str[5].substr(1,1) && d7==str[5].substr(2,1) && d8==str[5].substr(3,1)) ||
                    (d6==str[6].substr(1,1) && d7==str[6].substr(2,1) && d8==str[6].substr(3,1)) ||
                    (d1==str[0].substr(0,1) && d2==str[0].substr(1,1) && d3==str[0].substr(2,1) && d4==str[0].substr(3,1) &&
                     d5==str[0].substr(4,1) && d6==str[0].substr(5,1) && d7==str[0].substr(6,1) && d8==str[0].substr(7,1)) ||
                    (d1==str[1].substr(0,1) && d2==str[1].substr(1,1) && d3==str[1].substr(2,1) && d4==str[1].substr(3,1) &&
                     d5==str[1].substr(4,1) && d6==str[1].substr(5,1) && d7==str[1].substr(6,1) && d8==str[1].substr(7,1))

                  )
                    cvPutText(EndImage1,s1,cvPoint(60,40),&Font1,CV_RGB(255,0,0));
                else
				*/
                    cvPutText(EndImage2,s1,cvPoint(60,40),&Font1,CV_RGB(255,255,0));

                //
				FreeArray1(digit);
				//---
			}	
			 
			//-------------------------------------
			//畫格線-白框
			//Grid(Image, Grid_X1, Grid_X2, Grid_Y1, Grid_Y2, CV_RGB(255,255,255));
			//Grid(frame, Grid_X1, Grid_X2, Grid_Y1, Grid_Y2, CV_RGB(255,255,255));
			Grid(frame, Grid_X1, Grid_X2, Grid_Y1, Grid_Y2, CV_RGB(255,255,255));
			//畫格線-藍框
			Grid(EndImage2, VPoint[0], VPoint[V_tmp-1], HPoint[0], HPoint[H_tmp-1], CV_RGB(0,255,255));

			/*
			ProcImgCopy(frame,    BackImage, 0, 0);                    //左上
			ProcImgCopy(Image,    BackImage, 0, IMG_SIZE_Y);           //左下
			ProcImgCopy(EndImage2,BackImage, IMG_SIZE_X, 0);           //中上
			ProcImgCopy(EndImage1,BackImage, IMG_SIZE_X, IMG_SIZE_Y);  //中下
            */
			/*
			FILE *file;
			file=fopen("note.txt","w");
			for(int x=0; x<BackImage->width; x++)
			{
	    		for(int y=0; y<BackImage->height; y++)
		    	{
			        unsigned long tmp[3]={};
			        for(int c=0; c<BackImage->nChannels; c++)
			        {
			            tmp[c] = (unsigned char)BackImage->imageData[ y*BackImage->widthStep + x*BackImage->nChannels + c ];
			        }
			        fprintf(file,"(%d,%d,%d)(x:%d,y:%d)\n",tmp[2],tmp[1],tmp[0],x,y);
			    }
			}
			fclose(file);
			*/

			//cvNamedWindow("Image",1);
			//cvShowImage  ("Image",BackImage);

			//ImgTmp = cvCloneImage(EndImage1);

			cvNamedWindow("digital_image",1);
			cvMoveWindow ("digital_image", 0, 0 );
			//cvShowImage  ("digital_image",ImgTmpBGR);
			cvShowImage  ("digital_image",EndImage2);
            cvSetMouseCallback("digital_image",onMouse1,NULL); //滑鼠座標

            //
            //cvNamedWindow("image",1);
			//cvMoveWindow ("image", Image->width+5, 0 );
			//cvShowImage  ("image",EndImage1);
            
			//cvSetMouseCallback("image",onMouse2,NULL); //滑鼠座標

			//cvShowImage  ("image",Image);
			//cvShowImage  ("image",ImgTmpBGR);
            //


            /*
			sprintf_s(buffer,sizeof(buffer)"image%u.jpg",number);
			cvSaveImage(buffer,BackImage);
			number++;
            */

			char c = cvWaitKey(40);        //1000毫秒60幀圖像 (1幀圖像 = 16.6毫秒)

			if(c == 27)
			{
				cvReleaseCapture(&capture);
				exit(1); //按ESC退出
            }

			if(c == 32)
			{
				sprintf(buffer,"test01.png");
				cvSaveImage(buffer,EndImage2);
				cvWaitKey(0);    //space 暫停
			}

			cvReleaseImage(&ImgTmpYUV);
			cvReleaseImage(&ImgTmpBGR);
		}
	}

	cvReleaseImage(&frame);
	cvReleaseImage(&Image);
	cvReleaseImage(&EndImage);
	cvReleaseImage(&EndImage1);
	cvReleaseImage(&EndImage2);
	cvReleaseImage(&BackImage);
	cvReleaseImage(&OriImage);

	cvReleaseImage(&tmp);
    tmp1 = cvCreateImage(cvSize(0,0),8,3);
	cvReleaseImage(&tmp1);

	cvReleaseImage(&tmp2);
	tmp3 = cvCreateImage(cvSize(0,0),8,3);
    cvReleaseImage(&tmp3);
    cvReleaseImage(&tmp4);

	cvReleaseImage(&two_one_Image);
	cvReleaseCapture(&capture);
	cvDestroyWindow("Image");
	cvDestroyWindow("digital_Image");

	cvReleaseMat(&mat_roi);
	cvReleaseMat(&mat_roi1);

	exit(1);
}

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

void ProcImgCopy (IplImage *pSrc, IplImage *pDst, int DstBegX, int DstBegY)
{
	int SrcWidthStep = pSrc->widthStep;
	int DstWidthStep = pDst->widthStep;
	int DstChannels  = pDst->nChannels;
	for ( int h=0; h<pSrc->height; h++ )
	{
		memcpy(&pDst->imageData[(DstBegY+h)*DstWidthStep + DstBegX*DstChannels],
			&pSrc->imageData[h*SrcWidthStep],
			SrcWidthStep);
	}
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

int thrDarkGray_value_x(IplImage* pSrc, int VStart, int VEnd, int HStart, int HEnd)
{
	unsigned long Tmp=0;
	unsigned long nThrPixel = 0;
	unsigned long threshold = 0;
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

int ImgChange(IplImage* aaa,int k)
{
	int Mask_buf=5; //,k=1;
	IplImage* bbb=cvCreateImage(cvGetSize(aaa),IPL_DEPTH_8U,1);
	long Sumtemp=0;//ISUB用
	int j=k%sw;

	cvCvtColor(aaa, bbb, CV_BGR2GRAY);

	//if(bbb==NULL)printf("");

	for(int y=Mask_buf; y<bbb->height-Mask_buf; y++)
		for(int x=Mask_buf; x<bbb->width-Mask_buf; x++)
		{
			if(k>0)
				Sumtemp=abs(bbb->imageData[(x+y*bbb->width)]-Array_ISubTemp[x+y*bbb->width])+Sumtemp;
			Array_ISubTemp[x+y*bbb->width]=bbb->imageData[x+y*bbb->width];
		}

    Array_KsumISub[j]=Sumtemp;

    //以下移動平均
    cvReleaseImage(&bbb);

    if(k>=sw)
    {//IT用
        long  sum=0,MA=0;

        for(int i=sw-1;i>=0;i--)
            if(i!=j)
                sum=sum+ Array_KsumISub[i];

        MA=sum/(sw-1);
        //1模糊  0清晰
        if(MA>Array_KsumISub[j])
            return 0;
        else
            return 1;
    }
    else
        return 1;
}

void Cut_Line(IplImage* Img, int X, int Y, CvScalar Color)
{
    for(int x=0; x<Img->width; x=x+X)
        if(x>0)
            cvLine(Img, cvPoint(x,0), cvPoint(x,Img->width), Color, 1);

    for(int y=0; y<Img->height; y=y+Y)
        if(y>0)
            cvLine(Img, cvPoint(0,y), cvPoint(Img->height,y), Color, 1);
}

void White_Area(IplImage* img, int* m, int i, int x_start, int x_end, int y_start, int y_end)
{
	for(int x=x_start; x<x_end; x++)
		for(int y=y_start; y<y_end; y++)
			if( (unsigned char)img->imageData[y*img->widthStep + x*img->nChannels + 0] == (unsigned char)0xff )
				m[i]++;
}

void onMouse1(int event,int x,int y,int flags,void* param )  //原始圖座標
{
	int T;
	int TmpBGR_B=(unsigned char)ImgTmpBGR->imageData[ y*ImgTmpBGR->widthStep + x*ImgTmpBGR->nChannels + 0 ];
	int TmpBGR_G=(unsigned char)ImgTmpBGR->imageData[ y*ImgTmpBGR->widthStep + x*ImgTmpBGR->nChannels + 1 ];
	int TmpBGR_R=(unsigned char)ImgTmpBGR->imageData[ y*ImgTmpBGR->widthStep + x*ImgTmpBGR->nChannels + 2 ];
    int YUV_Gray=(unsigned char)ImgTmpYUV->imageData[ y*ImgTmpYUV->widthStep + x*ImgTmpYUV->nChannels + 0 ];
	if((unsigned char)EndImage2->imageData[ y*EndImage2->widthStep + x*EndImage2->nChannels + 0 ]==0xff)
	    T=1;
    else
        T=0;

    if(event==CV_EVENT_RBUTTONDOWN)
	{
        printf("\n");
        for(int i=x-1; i<=x+1; i++)
            for(int j=y-1; j<=y+1; j++)
            {
                int n=(unsigned char)ImgTmpBGR->imageData[ j*ImgTmpBGR->widthStep + i*ImgTmpBGR->nChannels + 0 ];
                int m=(unsigned char)ImgTmpBGR->imageData[ j*ImgTmpBGR->widthStep + i*ImgTmpBGR->nChannels + 1 ];
                int l=(unsigned char)ImgTmpBGR->imageData[ j*ImgTmpBGR->widthStep + i*ImgTmpBGR->nChannels + 2 ];
                int k=(unsigned char)ImgTmpYUV->imageData[ j*ImgTmpYUV->widthStep + i*ImgTmpYUV->nChannels + 0 ];
                int o;
                if((unsigned char)EndImage2->imageData[ j*EndImage2->widthStep + i*EndImage2->nChannels + 0 ]==0xff)
	                o=1;
                else
                    o=0;
                printf("%3d,%3d BW:%3d RGB:%3d,%3d,%3d T:%d\n", i,j,k,l,m,n,o);
            }
            //
            //VPoint[0]>0 && VPoint[V_tmp-1]>0 && HPoint[0]>0 && HPoint[H_tmp-1]>0
            IplImage* tmp_t = cvCreateImage(cvSize((VPoint[V_tmp-1]-VPoint[0]+1)*5,
                                                 (HPoint[H_tmp-1]-HPoint[0]+1)*5),
                                                  IPL_DEPTH_8U,3);
            IplImage* tmp_t1 = cvCreateImage(cvSize((VPoint[V_tmp-1]-VPoint[0]+1),
                                                 (HPoint[H_tmp-1]-HPoint[0]+1)),
                                                  IPL_DEPTH_8U,3);
													//高              寬

            CvMat* mat_roi_t1 = cvCreateMat((HPoint[H_tmp-1]-HPoint[0]+1),
                                            (VPoint[V_tmp-1]-VPoint[0]+1),CV_8UC3);
										//cvRect(X, Y, 寬, 高)
            CvRect rect_roi_t1 = cvRect( VPoint[0], HPoint[0], (VPoint[V_tmp-1]-VPoint[0]+1),
                                      (HPoint[H_tmp-1]-HPoint[0]+1));
            //從SRC影像中切割rect_roi位置和大小儲存在mat_roi陣列
            cvGetSubRect(EndImage2, mat_roi_t1, rect_roi_t1);
            //cvGetImage(陣列mat, 影像image)，將陣列轉成影像
            cvGetImage(mat_roi_t1, tmp_t1);
            //cvReleaseMat(&mat_roi_t);

            cvResize(tmp_t1, tmp_t, CV_INTER_NN);    //縮放來源影像到目標影像

            cvNamedWindow("Resize_image", 1);
			cvMoveWindow ("Resize_image", 0, EndImage2->height+30 );
			cvShowImage  ("Resize_image",tmp_t);

			cvReleaseImage(&tmp_t);
			cvReleaseImage(&tmp_t1);
			cvReleaseMat(&mat_roi_t1);
			
    }

	CvFont Font1=cvFont(1,1);	
	char s1[60];
	int2str(x,y,(int)T ,(int)TmpBGR_R,(int)TmpBGR_G,(int)TmpBGR_B,(int)YUV_Gray,s1);
	cvRectangle(EndImage2, cvPoint(0,210), cvPoint(319,210+30), CV_RGB(0,0,0),-1, 8, 0 ); //文字背景
	cvPutText(EndImage2,s1,cvPoint(0,210+20),&Font1,CV_RGB(255,255,255));
	cvShowImage("digital_image",EndImage2);
}

void onMouse2(int event,int x,int y,int flags,void* param )  //原始圖座標
{
	int TmpBGR_B=(unsigned char)OriImage->imageData[ y*OriImage->widthStep + x*OriImage->nChannels + 0 ];
	int TmpBGR_G=(unsigned char)OriImage->imageData[ y*OriImage->widthStep + x*OriImage->nChannels + 1 ];
	int TmpBGR_R=(unsigned char)OriImage->imageData[ y*OriImage->widthStep + x*OriImage->nChannels + 2 ];
	int YUV_Gray=(unsigned char)ImgTmpYUV->imageData[ y*ImgTmpYUV->widthStep + x*ImgTmpYUV->nChannels + 0 ];
	int T;
	if((unsigned char)EndImage2->imageData[ y*EndImage2->widthStep + x*EndImage2->nChannels + 0 ]==0xff)
	    T=1;
    else
        T=0;
	CvFont Font1=cvFont(1,1);
	char s1[60];
	int2str1(x,y,(int)TmpBGR_R,(int)TmpBGR_G,(int)TmpBGR_B,(int)YUV_Gray,(int)T,s1);
	cvRectangle(OriImage, cvPoint(0,210), cvPoint(319,210+30), CV_RGB(0,0,0),-1, 8, 0 ); //文字背景
	cvPutText(OriImage,s1,cvPoint(0,210+20),&Font1,CV_RGB(255,255,255));
	cvShowImage("image",OriImage);
}

void int2str(int i,int j,int k,int l, int m, int n, int o, char *s)
{
	sprintf(s,"%d,%d BW:%d RGB:%d,%d,%d Y:%d",i,j,k,l,m,n,o);
}

void int2str1(int i,int j,int k,int l, int m, int n, int o,char *s)
{
	sprintf(s,"(%d,%d)(RGB:%d,%d,%d)(Y:%d T:%d)",i,j,k,l,m,n,o);
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
					if(tmp[left_x][y] == 0xff)       //x-1,y
					{
						tmp[left_x][y] = label;
						nPixel++;
					}
					if(tmp[left_x][down_y] == 0xff)       //x-1,y+1
					{
						tmp[left_x][down_y] = label;
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

                    if(tmp[right_x][up_y] == 0xff)   //x+1,y-1
                    {
                        tmp[right_x][up_y] = label;
                        nPixel++;
                    }
					if(tmp[right_x][y] == 0xff)      //x+1,y
					{
						tmp[right_x][y] = label;
						nPixel++;
					}
                    if(tmp[right_x][down_y] == 0xff) //x+1,y+1
                    {
                        tmp[right_x][down_y] = label;
                        nPixel++;
                    }                                                          
                }
            }
        }
        if( nPixel==0 ) break;
    }
}

void pixel_link_1(char** const tmp, int v1, int v2, int h1, int h2, int x1, int y1, int label)
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

					if(tmp[left_x][up_y] == 0)    //x-1,y-1
					{
						tmp[left_x][up_y] = label;
						nPixel++;
					}
					if(tmp[left_x][y] == 0)       //x-1,y
					{
						tmp[left_x][y] = label;
						nPixel++;
					}
					if(tmp[left_x][down_y] == 0)       //x-1,y+1
					{
						tmp[left_x][down_y] = label;
						nPixel++;
					}
					if(tmp[x][up_y] == 0)         //x,y-1
					{
						tmp[x][up_y] = label;
						nPixel++;
					}
					if(tmp[x][down_y] == 0)       //x,y+1
					{
						tmp[x][down_y] = label;
						nPixel++;
					}
					if(tmp[right_x][up_y] == 0)   //x+1,y-1
					{
						tmp[right_x][up_y] = label;
						nPixel++;
					}
					if(tmp[right_x][y] == 0)      //x+1,y
					{
						tmp[right_x][y] = label;
						nPixel++;
					}
					if(tmp[right_x][down_y] == 0) //x+1,y+1
					{
						tmp[right_x][down_y] = label;
						nPixel++;
					}					
				}
			}
		}
		if( nPixel==0 ) break;
	}
}

void FreeArray1(int*& ImgArray)
{	
	if ( ImgArray != NULL )
	{
		delete [] ImgArray;
		ImgArray = NULL;
	}	
}

void FreeArray2(char**& ImgArray, int n)
{	
	for ( int row=0; row<n; row++ )
	{
		if ( ImgArray[row] != NULL )
		{
			delete [] ImgArray[n];
			ImgArray[n] = NULL;
		}
	}	
	if ( ImgArray != NULL )
	{
		delete [] ImgArray;
		ImgArray = NULL;
	}	
}


//init
bool Init_Image(char**& ImgArray, int n)
{	
	if ( ImgArray != NULL )
		FreeArray2(ImgArray, n);
	
	try
	{
		ImgArray = new char* [n];
		for ( int row=0; row<n; row++ )
			ImgArray[row] = new char [n];
	}
	catch ( ... )
	{
		FreeArray2(ImgArray, n);
		return false;
	}
	
	return true;
}

//判斷 ptr
void IsRegion(queue<long>& queue_FIFO, char** const ImgArray,
	const long row, const long col, const long length) //Black Point
{
	if ( row < 0 || row >= length )
		return;
	if ( col < 0 || col >= length )
		return;
	if ( ImgArray[row][col] == COLOR_BG )
	{
		ImgArray[row][col] = COLOR_FG;
		queue_FIFO.push(row*length + col);
	}
}

void Mark_Region (char** const ImgArray, const long ptr, const long length)  // Mark Region
{
	long ptrThis = ptr;
	long row = ptrThis / length;
	long col = ptrThis % length;
	ImgArray[row][col] = COLOR_FG;

	queue<long> queue_FIFO;
	queue_FIFO.push(ptrThis);
	while ( queue_FIFO.size() )
	{
		long neiRow, neiCol;
		long ptrThis = queue_FIFO.front();
		row = ptrThis / length;
		col = ptrThis % length;
		queue_FIFO.pop();
		// Up
		neiRow = row;		neiCol = col - 1;
		IsRegion(queue_FIFO, ImgArray, neiRow, neiCol, length);
		// Down
		neiRow = row;		neiCol = col + 1;
		IsRegion(queue_FIFO, ImgArray, neiRow, neiCol, length);
		// Right
		neiRow = row + 1;	neiCol = col;
		IsRegion(queue_FIFO, ImgArray, neiRow, neiCol, length);
		// Left
		neiRow = row - 1;	neiCol = col;
		IsRegion(queue_FIFO, ImgArray, neiRow, neiCol, length);
	}
}

//print
void PrintImage (char** const ImgArray, long n)
{
	for ( int row=0; row<n; row++ )
	{
		for ( int col=0; col<n; col++ )
			printf("%2d ",ImgArray[row][col]);		
		printf("\n");
	}
	printf("\n");
}
