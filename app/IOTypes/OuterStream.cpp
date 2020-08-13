#include "OuterStream.hpp"
#include <QtCharts/QXYSeries>

#include <ftd2xx.h>
#include <LibFT4222.h>
#define BUFFER_SIZE 16384

FT_HANDLE ftHandle = nullptr;
std::vector< FT_DEVICE_LIST_INFO_NODE > g_FTAllDevList;
std::vector< FT_DEVICE_LIST_INFO_NODE > g_FT4222DevList;

void WriteDataToFile(FILE *fp,unsigned char c) {
	static int i = 0;
	static int num = 0;
	//printf("%x ", c);
	num |= (c << i*8);
	i++;
	if (i == 4) {
		//printf("%x\n", num);
		fprintf(fp, "%d\n", num);
		num = 0;
		i = 0;
	}
}

inline std::string DeviceFlagToString(DWORD flags)
{
	std::string msg;
	msg += (flags & 0x1) ? "DEVICE_OPEN" : "DEVICE_CLOSED";
	msg += ", ";
	msg += (flags & 0x2) ? "High-speed USB" : "Full-speed USB";
	return msg;
}

void ListFtUsbDevices()
{
	FT_STATUS ftStatus = 0;

	DWORD numOfDevices = 0;
	ftStatus = FT_CreateDeviceInfoList(&numOfDevices);

	for (DWORD iDev = 0; iDev<numOfDevices; ++iDev)
	{
		FT_DEVICE_LIST_INFO_NODE devInfo;
		memset(&devInfo, 0, sizeof(devInfo));

		ftStatus = FT_GetDeviceInfoDetail(iDev, &devInfo.Flags, &devInfo.Type, &devInfo.ID, &devInfo.LocId,
			devInfo.SerialNumber,
			devInfo.Description,
			&devInfo.ftHandle);

		if (FT_OK == ftStatus)
		{
			printf("Dev %lu:\n", iDev);
			printf("  Flags= 0x%x, (%s)\n", devInfo.Flags, DeviceFlagToString(devInfo.Flags).c_str());
			printf("  Type= 0x%x\n", devInfo.Type);
			printf("  ID= 0x%x\n", devInfo.ID);
			printf("  LocId= 0x%x\n", devInfo.LocId);
			printf("  SerialNumber= %s\n", devInfo.SerialNumber);
			printf("  Description= %s\n", devInfo.Description);
			printf("  ftHandle= 0x%x\n", devInfo.ftHandle);

			const std::string desc = devInfo.Description;
			g_FTAllDevList.push_back(devInfo);

			if (desc == "FT4222" || desc == "FT4222 A")
			{
				g_FT4222DevList.push_back(devInfo);
			}
		}
	}
}


OuterStream::OuterStream(QObject *parent) :
	QIODevice(parent)
{

}

bool OuterStream::open(OpenMode mode) {

	FT4222_STATUS ft4222_status;

	ListFtUsbDevices();

	if (g_FT4222DevList.empty()) {
		printf("No FT4222 device is found!\n");
		return false;
	}

	FT_STATUS ftStatus;
	ftStatus = FT_OpenEx((PVOID)g_FT4222DevList[0].LocId, FT_OPEN_BY_LOCATION, &ftHandle);
	if (FT_OK != ftStatus)
	{
		printf("Open a FT4222 device failed!\n");
		return false;
	}

	ft4222_status = FT4222_SetClock(ftHandle, SYS_CLK_80);
	if (FT_OK != ftStatus)
	{
		printf("FT4222_SetClock failed!\n");
		return false;
	}

	ftStatus = FT_SetUSBParameters(ftHandle, 4 * 1024, 0);
	if (FT_OK != ftStatus)
	{
		printf("FT_SetUSBParameters failed!\n");
		return false;
	}

	ft4222_status = FT4222_SPISlave_InitEx(ftHandle, SPI_SLAVE_NO_PROTOCOL);
	if (FT4222_OK != ft4222_status)
	{
		printf("Init FT4222 as SPI slave device failed!\n");
		return false;
	}

	ft4222_status = FT4222_SPISlave_SetMode(ftHandle, CLK_IDLE_LOW, CLK_TRAILING);
	if (FT4222_OK != ft4222_status)
	{
		printf("Set Node FT4222 as SPI slave device failed!\n");
		return false;
	}

	ft4222_status = FT4222_SPI_SetDrivingStrength(ftHandle, DS_4MA, DS_4MA, DS_4MA);
	if (FT4222_OK != ft4222_status)
	{
		printf("FT4222_SPI_SetDrivingStrength failed!\n");
		return false;
	}

	printf("waiting for data...\n");

	return true;
}

quint16 OuterStream::readExternalData(quint8 *data, quint16 maxlen){
	quint16 readLen(0);
	FT4222_STATUS ft4222_status = FT4222_SPISlave_GetRxStatus(ftHandle, &maxlen);
	if (ft4222_status == FT4222_OK){
		qDebug()<<"RXSize: "<<maxlen;
		ft4222_status = FT4222_SPISlave_Read(ftHandle, data, BUFFER_SIZE, &readLen);
	}

	return readLen;
}

void OuterStream::close(){
	FT_STATUS ftStatus = FT_Close(ftHandle);
	qDebug()<<"Close status"<<(ftStatus == FT_OK);
}
