#include "includes.h"

class grav2d_cube
{
public:
	grav2d_cube(){}
	~grav2d_cube(){}
	int routine(char*,char*,char*);
	int initCubes(char*);
	int initObs(char*);
	void outObs();
	void calG();
	void calGx();
	void calGy();
	void calGz();
private:
	int obsNum, cubeNum;
	cubeArray modCube;
	obspointArray obsPoint;
};

int grav2d_cube::routine(char* calType,char* obsPara,char* modPara)
{
	if (initCubes(modPara)) return -1;
	if (initObs(obsPara)) return -1;
	if (!strcmp(calType,"gravity")) calG();
	else if (!strcmp(calType,"gx")) calGx();
	else if (!strcmp(calType,"gy")) calGy();
	else if (!strcmp(calType,"gz")) calGz();
	else
	{
		cerr << BOLDRED << "error ==> " << RESET << "unknown calculation type: " << calType << endl;
		return -1;
	}
	outObs();
	return 0;
}

int grav2d_cube::initCubes(char* para)
{
	cube temp_cube;
	string temp_str;
	stringstream temp_ss;

	if (7 == sscanf(para,"%lf/%lf/%lf/%lf/%lf/%lf/%lf",
		&temp_cube.cen.x,&temp_cube.cen.y,&temp_cube.cen.z,&temp_cube.dx,&temp_cube.dy,&temp_cube.dz,&temp_cube.rho))
	{
		modCube.push_back(temp_cube);
	}
	else
	{
		ifstream infile;
		if (open_infile(infile,para)) return -1;

		while(getline(infile,temp_str))
		{
			if (*(temp_str.begin()) == '#') continue;
			else
			{
				//按每行7个数据解析 初始化为用于正演的观测点
				if (7 == sscanf(temp_str.c_str(),"%lf/%lf/%lf/%lf/%lf/%lf/%lf",
					&temp_cube.cen.x,&temp_cube.cen.y,&temp_cube.cen.z,&temp_cube.dx,&temp_cube.dy,&temp_cube.dz,&temp_cube.rho))
					modCube.push_back(temp_cube);
				else
				{
					cerr << BOLDYELLOW << "ignored ==> " << RESET << "wrong input: " << temp_str << endl;
					continue;
				}
			}
		}
		infile.close();
	}

	if (modCube.empty())
	{
		cerr << BOLDRED << "error ==> " << RESET << "fail to initial cubes with the parameter: " << para << endl;
		return -1;
	}
	else cubeNum = modCube.size();
	return 0;
}

int grav2d_cube::initObs(char* para)
{
	obspoint temp_obs;
	string temp_str;
	stringstream temp_ss;
	double x,y;
	double xmin,xmax,ymin,ymax;
	double xs,xe,ys,ye,eleva,dx,dy;

	//按格式解析参数 初始化观测位置 用于正演计算
	if (7 == sscanf(para,"%lf/%lf/%lf/%lf/%lf/%lf/%lf",&xs,&dx,&xe,&ys,&dy,&ye,&eleva))
	{
		xmin = MIN(xs,xe); xmax = MAX(xs,xe);
		ymin = MIN(ys,ye); ymax = MAX(ys,ye);

		y = ys;
		while(y >= ymin && y <= ymax)
		{
			x = xs;
			while(x >= xmin && x <= xmax)
			{
				temp_obs.id = obsPoint.size();
				temp_obs.x = x; temp_obs.y = y; temp_obs.z = -1.0*eleva;
				temp_obs.val = 0.0;
				obsPoint.push_back(temp_obs);
				x += dx;
			}
			y += dy;
		}
	}
	//解析失败 按文件读入 用于反演使用或者正演计算
	else
	{
		ifstream infile;
		if (open_infile(infile,para)) return -1;

		while(getline(infile,temp_str))
		{
			if (*(temp_str.begin()) == '#') continue;
			else
			{
				//按每行3个数据解析 初始化为用于正演的观测点
				if (3 == sscanf(temp_str.c_str(),"%lf %lf %lf",&temp_obs.x,&temp_obs.y,&temp_obs.z))
				{
					temp_obs.z *= -1.0;
					temp_obs.val = 0.0;
					temp_obs.id = obsPoint.size();
					obsPoint.push_back(temp_obs);
				}
				else
				{
					cerr << BOLDYELLOW << "ignored ==> " << RESET << "wrong input: " << temp_str << endl;
					continue;
				}
			}
		}
		infile.close();
	}

	if (obsPoint.empty())
	{
		cerr << BOLDRED << "error ==> " << RESET << "fail to initial observations with the parameter: " << para << endl;
		return -1;
	}
	else obsNum = obsPoint.size();
	return 0;
}

void grav2d_cube::outObs()
{
	cout << "# This file is generated by grav2d-cube. Use -h to see options" << endl;
	cout << "# x(m) y(m) ele(m) obs-val(mGal)" << endl;
	for (int i = 0; i < obsNum; i++)
		cout << obsPoint[i].x << " " << obsPoint[i].y << " " << -1.0*obsPoint[i].z << " " << setprecision(16) << obsPoint[i].val << endl;
	return;
}

void grav2d_cube::calG()
{
	int i,j;
	double x1,x2,y1,y2,z1,z2;
	double R222,R122,R212,R112,R221,R121,R211,R111;
	double G222,G122,G212,G112,G221,G121,G211,G111;

	for (j = 0; j < cubeNum; j++)
	{
		x1 = modCube[j].cen.x - 0.5*modCube[j].dx; x2 = modCube[j].cen.x + 0.5*modCube[j].dx;
		y1 = modCube[j].cen.y - 0.5*modCube[j].dy; y2 = modCube[j].cen.y + 0.5*modCube[j].dy;
		z1 = modCube[j].cen.z - 0.5*modCube[j].dz; z2 = modCube[j].cen.z + 0.5*modCube[j].dz;

#pragma omp parallel for private(i,R222,R122,R212,R112,R221,R121,R211,R111,G222,G122,G212,G112,G221,G121,G211,G111) shared(x1,x2,y1,y2,z1,z2) schedule(guided)
		for (i = 0; i < obsNum; i++)
		{
			R222=sqrt((x2-obsPoint[i].x)*(x2-obsPoint[i].x)+(y2-obsPoint[i].y)*(y2-obsPoint[i].y)+(z2-obsPoint[i].z)*(z2-obsPoint[i].z));
			R122=sqrt((x1-obsPoint[i].x)*(x1-obsPoint[i].x)+(y2-obsPoint[i].y)*(y2-obsPoint[i].y)+(z2-obsPoint[i].z)*(z2-obsPoint[i].z));
			R212=sqrt((x2-obsPoint[i].x)*(x2-obsPoint[i].x)+(y1-obsPoint[i].y)*(y1-obsPoint[i].y)+(z2-obsPoint[i].z)*(z2-obsPoint[i].z));
			R112=sqrt((x1-obsPoint[i].x)*(x1-obsPoint[i].x)+(y1-obsPoint[i].y)*(y1-obsPoint[i].y)+(z2-obsPoint[i].z)*(z2-obsPoint[i].z));
			R221=sqrt((x2-obsPoint[i].x)*(x2-obsPoint[i].x)+(y2-obsPoint[i].y)*(y2-obsPoint[i].y)+(z1-obsPoint[i].z)*(z1-obsPoint[i].z));
			R121=sqrt((x1-obsPoint[i].x)*(x1-obsPoint[i].x)+(y2-obsPoint[i].y)*(y2-obsPoint[i].y)+(z1-obsPoint[i].z)*(z1-obsPoint[i].z));
			R211=sqrt((x2-obsPoint[i].x)*(x2-obsPoint[i].x)+(y1-obsPoint[i].y)*(y1-obsPoint[i].y)+(z1-obsPoint[i].z)*(z1-obsPoint[i].z));
			R111=sqrt((x1-obsPoint[i].x)*(x1-obsPoint[i].x)+(y1-obsPoint[i].y)*(y1-obsPoint[i].y)+(z1-obsPoint[i].z)*(z1-obsPoint[i].z));

			G222=(x2-obsPoint[i].x)*log((y2-obsPoint[i].y)+R222)+(y2-obsPoint[i].y)*log((x2-obsPoint[i].x)+R222)+(z2-obsPoint[i].z)*arctg((z2-obsPoint[i].z)*R222/(x2-obsPoint[i].x)/(y2-obsPoint[i].y));
			G122=(x1-obsPoint[i].x)*log((y2-obsPoint[i].y)+R122)+(y2-obsPoint[i].y)*log((x1-obsPoint[i].x)+R122)+(z2-obsPoint[i].z)*arctg((z2-obsPoint[i].z)*R122/(x1-obsPoint[i].x)/(y2-obsPoint[i].y));
			G212=(x2-obsPoint[i].x)*log((y1-obsPoint[i].y)+R212)+(y1-obsPoint[i].y)*log((x2-obsPoint[i].x)+R212)+(z2-obsPoint[i].z)*arctg((z2-obsPoint[i].z)*R212/(x2-obsPoint[i].x)/(y1-obsPoint[i].y));
			G112=(x1-obsPoint[i].x)*log((y1-obsPoint[i].y)+R112)+(y1-obsPoint[i].y)*log((x1-obsPoint[i].x)+R112)+(z2-obsPoint[i].z)*arctg((z2-obsPoint[i].z)*R112/(x1-obsPoint[i].x)/(y1-obsPoint[i].y));
			G221=(x2-obsPoint[i].x)*log((y2-obsPoint[i].y)+R221)+(y2-obsPoint[i].y)*log((x2-obsPoint[i].x)+R221)+(z1-obsPoint[i].z)*arctg((z1-obsPoint[i].z)*R221/(x2-obsPoint[i].x)/(y2-obsPoint[i].y));
			G121=(x1-obsPoint[i].x)*log((y2-obsPoint[i].y)+R121)+(y2-obsPoint[i].y)*log((x1-obsPoint[i].x)+R121)+(z1-obsPoint[i].z)*arctg((z1-obsPoint[i].z)*R121/(x1-obsPoint[i].x)/(y2-obsPoint[i].y));
			G211=(x2-obsPoint[i].x)*log((y1-obsPoint[i].y)+R211)+(y1-obsPoint[i].y)*log((x2-obsPoint[i].x)+R211)+(z1-obsPoint[i].z)*arctg((z1-obsPoint[i].z)*R211/(x2-obsPoint[i].x)/(y1-obsPoint[i].y));
			G111=(x1-obsPoint[i].x)*log((y1-obsPoint[i].y)+R111)+(y1-obsPoint[i].y)*log((x1-obsPoint[i].x)+R111)+(z1-obsPoint[i].z)*arctg((z1-obsPoint[i].z)*R111/(x1-obsPoint[i].x)/(y1-obsPoint[i].y));

			obsPoint[i].val += -1.0*G0*(G222-G122-G212+G112-G221+G121+G211-G111)*modCube[j].rho;
		}
	}
	return;
}

void grav2d_cube::calGx()
{
	int i,j;
	double x1,x2,y1,y2,z1,z2;
	double R222,R122,R212,R112,R221,R121,R211,R111;
	double G222,G122,G212,G112,G221,G121,G211,G111;

	for (j = 0; j < cubeNum; j++)
	{
		x1 = modCube[j].cen.x - 0.5*modCube[j].dx; x2 = modCube[j].cen.x + 0.5*modCube[j].dx;
		y1 = modCube[j].cen.y - 0.5*modCube[j].dy; y2 = modCube[j].cen.y + 0.5*modCube[j].dy;
		z1 = modCube[j].cen.z - 0.5*modCube[j].dz; z2 = modCube[j].cen.z + 0.5*modCube[j].dz;

#pragma omp parallel for private(i,R222,R122,R212,R112,R221,R121,R211,R111,G222,G122,G212,G112,G221,G121,G211,G111) shared(x1,x2,y1,y2,z1,z2) schedule(guided)
		for (i = 0; i < obsNum; i++)
		{
			R222=sqrt((x2-obsPoint[i].x)*(x2-obsPoint[i].x)+(y2-obsPoint[i].y)*(y2-obsPoint[i].y)+(z2-obsPoint[i].z)*(z2-obsPoint[i].z));
			R122=sqrt((x1-obsPoint[i].x)*(x1-obsPoint[i].x)+(y2-obsPoint[i].y)*(y2-obsPoint[i].y)+(z2-obsPoint[i].z)*(z2-obsPoint[i].z));
			R212=sqrt((x2-obsPoint[i].x)*(x2-obsPoint[i].x)+(y1-obsPoint[i].y)*(y1-obsPoint[i].y)+(z2-obsPoint[i].z)*(z2-obsPoint[i].z));
			R112=sqrt((x1-obsPoint[i].x)*(x1-obsPoint[i].x)+(y1-obsPoint[i].y)*(y1-obsPoint[i].y)+(z2-obsPoint[i].z)*(z2-obsPoint[i].z));
			R221=sqrt((x2-obsPoint[i].x)*(x2-obsPoint[i].x)+(y2-obsPoint[i].y)*(y2-obsPoint[i].y)+(z1-obsPoint[i].z)*(z1-obsPoint[i].z));
			R121=sqrt((x1-obsPoint[i].x)*(x1-obsPoint[i].x)+(y2-obsPoint[i].y)*(y2-obsPoint[i].y)+(z1-obsPoint[i].z)*(z1-obsPoint[i].z));
			R211=sqrt((x2-obsPoint[i].x)*(x2-obsPoint[i].x)+(y1-obsPoint[i].y)*(y1-obsPoint[i].y)+(z1-obsPoint[i].z)*(z1-obsPoint[i].z));
			R111=sqrt((x1-obsPoint[i].x)*(x1-obsPoint[i].x)+(y1-obsPoint[i].y)*(y1-obsPoint[i].y)+(z1-obsPoint[i].z)*(z1-obsPoint[i].z));

			G222=log((x2-obsPoint[i].x)+R222);
			G122=log((x1-obsPoint[i].x)+R122);
			G212=log((x2-obsPoint[i].x)+R212);
			G112=log((x1-obsPoint[i].x)+R112);
			G221=log((x2-obsPoint[i].x)+R221);
			G121=log((x1-obsPoint[i].x)+R121);
			G211=log((x2-obsPoint[i].x)+R211);
			G111=log((x1-obsPoint[i].x)+R111);

			obsPoint[i].val += 1.0e+4*G0*(G222-G122-G212+G112-G221+G121+G211-G111)*modCube[j].rho;
		}
	}
	return;
}

void grav2d_cube::calGy()
{
	int i,j;
	double x1,x2,y1,y2,z1,z2;
	double R222,R122,R212,R112,R221,R121,R211,R111;
	double G222,G122,G212,G112,G221,G121,G211,G111;

	for (j = 0; j < cubeNum; j++)
	{
		x1 = modCube[j].cen.x - 0.5*modCube[j].dx; x2 = modCube[j].cen.x + 0.5*modCube[j].dx;
		y1 = modCube[j].cen.y - 0.5*modCube[j].dy; y2 = modCube[j].cen.y + 0.5*modCube[j].dy;
		z1 = modCube[j].cen.z - 0.5*modCube[j].dz; z2 = modCube[j].cen.z + 0.5*modCube[j].dz;

#pragma omp parallel for private(i,R222,R122,R212,R112,R221,R121,R211,R111,G222,G122,G212,G112,G221,G121,G211,G111) shared(x1,x2,y1,y2,z1,z2) schedule(guided)
		for (i = 0; i < obsNum; i++)
		{
			R222=sqrt((x2-obsPoint[i].x)*(x2-obsPoint[i].x)+(y2-obsPoint[i].y)*(y2-obsPoint[i].y)+(z2-obsPoint[i].z)*(z2-obsPoint[i].z));
			R122=sqrt((x1-obsPoint[i].x)*(x1-obsPoint[i].x)+(y2-obsPoint[i].y)*(y2-obsPoint[i].y)+(z2-obsPoint[i].z)*(z2-obsPoint[i].z));
			R212=sqrt((x2-obsPoint[i].x)*(x2-obsPoint[i].x)+(y1-obsPoint[i].y)*(y1-obsPoint[i].y)+(z2-obsPoint[i].z)*(z2-obsPoint[i].z));
			R112=sqrt((x1-obsPoint[i].x)*(x1-obsPoint[i].x)+(y1-obsPoint[i].y)*(y1-obsPoint[i].y)+(z2-obsPoint[i].z)*(z2-obsPoint[i].z));
			R221=sqrt((x2-obsPoint[i].x)*(x2-obsPoint[i].x)+(y2-obsPoint[i].y)*(y2-obsPoint[i].y)+(z1-obsPoint[i].z)*(z1-obsPoint[i].z));
			R121=sqrt((x1-obsPoint[i].x)*(x1-obsPoint[i].x)+(y2-obsPoint[i].y)*(y2-obsPoint[i].y)+(z1-obsPoint[i].z)*(z1-obsPoint[i].z));
			R211=sqrt((x2-obsPoint[i].x)*(x2-obsPoint[i].x)+(y1-obsPoint[i].y)*(y1-obsPoint[i].y)+(z1-obsPoint[i].z)*(z1-obsPoint[i].z));
			R111=sqrt((x1-obsPoint[i].x)*(x1-obsPoint[i].x)+(y1-obsPoint[i].y)*(y1-obsPoint[i].y)+(z1-obsPoint[i].z)*(z1-obsPoint[i].z));

			G222=log((y2-obsPoint[i].y)+R222);
			G122=log((y2-obsPoint[i].y)+R122);
			G212=log((y1-obsPoint[i].y)+R212);
			G112=log((y1-obsPoint[i].y)+R112);
			G221=log((y2-obsPoint[i].y)+R221);
			G121=log((y2-obsPoint[i].y)+R121);
			G211=log((y1-obsPoint[i].y)+R211);
			G111=log((y1-obsPoint[i].y)+R111);

			obsPoint[i].val += 1.0e+4*G0*(G222-G122-G212+G112-G221+G121+G211-G111)*modCube[j].rho;
		}
	}
	return;
}

void grav2d_cube::calGz()
{
	int i,j;
	double x1,x2,y1,y2,z1,z2;
	double R222,R122,R212,R112,R221,R121,R211,R111;
	double G222,G122,G212,G112,G221,G121,G211,G111;

	for (j = 0; j < cubeNum; j++)
	{
		x1 = modCube[j].cen.x - 0.5*modCube[j].dx; x2 = modCube[j].cen.x + 0.5*modCube[j].dx;
		y1 = modCube[j].cen.y - 0.5*modCube[j].dy; y2 = modCube[j].cen.y + 0.5*modCube[j].dy;
		z1 = modCube[j].cen.z - 0.5*modCube[j].dz; z2 = modCube[j].cen.z + 0.5*modCube[j].dz;

#pragma omp parallel for private(i,R222,R122,R212,R112,R221,R121,R211,R111,G222,G122,G212,G112,G221,G121,G211,G111) shared(x1,x2,y1,y2,z1,z2) schedule(guided)
		for (i = 0; i < obsNum; i++)
		{
			R222=sqrt((x2-obsPoint[i].x)*(x2-obsPoint[i].x)+(y2-obsPoint[i].y)*(y2-obsPoint[i].y)+(z2-obsPoint[i].z)*(z2-obsPoint[i].z));
			R122=sqrt((x1-obsPoint[i].x)*(x1-obsPoint[i].x)+(y2-obsPoint[i].y)*(y2-obsPoint[i].y)+(z2-obsPoint[i].z)*(z2-obsPoint[i].z));
			R212=sqrt((x2-obsPoint[i].x)*(x2-obsPoint[i].x)+(y1-obsPoint[i].y)*(y1-obsPoint[i].y)+(z2-obsPoint[i].z)*(z2-obsPoint[i].z));
			R112=sqrt((x1-obsPoint[i].x)*(x1-obsPoint[i].x)+(y1-obsPoint[i].y)*(y1-obsPoint[i].y)+(z2-obsPoint[i].z)*(z2-obsPoint[i].z));
			R221=sqrt((x2-obsPoint[i].x)*(x2-obsPoint[i].x)+(y2-obsPoint[i].y)*(y2-obsPoint[i].y)+(z1-obsPoint[i].z)*(z1-obsPoint[i].z));
			R121=sqrt((x1-obsPoint[i].x)*(x1-obsPoint[i].x)+(y2-obsPoint[i].y)*(y2-obsPoint[i].y)+(z1-obsPoint[i].z)*(z1-obsPoint[i].z));
			R211=sqrt((x2-obsPoint[i].x)*(x2-obsPoint[i].x)+(y1-obsPoint[i].y)*(y1-obsPoint[i].y)+(z1-obsPoint[i].z)*(z1-obsPoint[i].z));
			R111=sqrt((x1-obsPoint[i].x)*(x1-obsPoint[i].x)+(y1-obsPoint[i].y)*(y1-obsPoint[i].y)+(z1-obsPoint[i].z)*(z1-obsPoint[i].z));

			G222=atan((x2-obsPoint[i].x)*(y2-obsPoint[i].y)/(R222*(z2-obsPoint[i].z)));
			G122=atan((x1-obsPoint[i].x)*(y2-obsPoint[i].y)/(R122*(z2-obsPoint[i].z)));
			G212=atan((x2-obsPoint[i].x)*(y1-obsPoint[i].y)/(R212*(z2-obsPoint[i].z)));
			G112=atan((x1-obsPoint[i].x)*(y1-obsPoint[i].y)/(R112*(z2-obsPoint[i].z)));
			G221=atan((x2-obsPoint[i].x)*(y2-obsPoint[i].y)/(R221*(z1-obsPoint[i].z)));
			G121=atan((x1-obsPoint[i].x)*(y2-obsPoint[i].y)/(R121*(z1-obsPoint[i].z)));
			G211=atan((x2-obsPoint[i].x)*(y1-obsPoint[i].y)/(R211*(z1-obsPoint[i].z)));
			G111=atan((x1-obsPoint[i].x)*(y1-obsPoint[i].y)/(R111*(z1-obsPoint[i].z)));

			obsPoint[i].val += -1.0e+4*G0*(G222-G122-G212+G112-G221+G121+G211-G111)*modCube[j].rho;
		}
	}
	return;
}