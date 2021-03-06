/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
 /*
  * Copyright (c) 2018 Julian Heuking <J.Heuking@beckhoff.com>
  */

#include "zfsinstaller.h"

// Usage:
//	zfsinstaller install [inf] [installFolder] (defaults to something like %ProgramFiles%\ZFS)
//	zfsinstaller uninstall [inf] (could default to something like %ProgramFiles%\ZFS\ZFSin.inf)
//
//


int main(int argc, char* argv[])
{
	if (argc <= 2) {
		fprintf(stderr, "too few arguments \n");
		printUsage();
		return ERROR_BAD_ARGUMENTS;
	}
	if (argc > 3) {
		fprintf(stderr, "too many arguments \n");
		printUsage();
		return ERROR_BAD_ARGUMENTS;
	}

	if (strcmp(argv[1], "install") == 0) {
		zfs_install(argv[2]);
		fprintf(stderr, "Installation done.");
	} else if(strcmp(argv[1], "uninstall") == 0) {
		zfs_uninstall(argv[2]);
		fprintf(stderr, "Uninstall done. Reboot required.");
	} else {
		fprintf(stderr, "unknown argument %s\n", argv[1]);
		printUsage();
		return ERROR_BAD_ARGUMENTS;
	}
}

void printUsage() {
	fprintf(stderr, "Usage:\n\n");
	fprintf(stderr, "Install driver per INF DefaultInstall section:\n");
	fprintf(stderr, "zfsinstaller install [inf path]\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "Uninstall driver per INF DefaultUninstall section:\n");
	fprintf(stderr, "zfsinstaller uninstall [inf path]\n");
}

wchar_t *charToWchar(char * str) {
	const size_t cSize = strlen(str) + 1;
	wchar_t* wc = new wchar_t[cSize];
	size_t ret;
	mbstowcs_s(&ret, wc, cSize, str, MAX_PATH);
	wprintf(L"%s: return %s\n", __func__, wc);
	return wc;
}

int zfs_install(char *inf_path) {
	// 128+4	If a reboot of the computer is necessary, ask the user for permission before rebooting.
	return installInf("DefaultInstall 132 ", inf_path); 
}

int zfs_uninstall(char *inf_path) {
	int ret = 0;

	send_zfs_ioc_unregister_fs();

	// 128+2	Always ask the users if they want to reboot.
	ret = installInf("DefaultUninstall 130 ", inf_path);

	return ret;
}


int installInf(const char *cmd, char *inf_path) {

#ifdef _DEBUG
	system("sc query ZFSin");
	fprintf(stderr, "\n\n");
#endif

	size_t len = strlen(cmd) + strlen(inf_path) + 1;
	size_t sz = 0;
	char buf[MAX_PATH];
	wchar_t wc_buf[MAX_PATH];

	sprintf_s(buf, "%s%s", cmd, inf_path);
	fprintf(stderr, "%s\n", buf);

	mbstowcs_s(&sz, wc_buf, len, buf, MAX_PATH);

	InstallHinfSection(
		NULL,
		NULL,
		wc_buf,
		0
	);

#ifdef _DEBUG
	system("sc query ZFSin");
#endif
	
	return 0;
	// if we want to have some more control on installation, we need to get
	// a bit deeper into the setupapi, something like the following...

	/*HINF inf = SetupOpenInfFile(
		L"C:\\master_test\\ZFSin\\ZFSin.inf",//PCWSTR FileName,
		NULL,//PCWSTR InfClass,
		INF_STYLE_WIN4,//DWORD  InfStyle,
		0//PUINT  ErrorLine
	);

	if (!inf) {
		std::cout << "SetupOpenInfFile failed, err " << GetLastError() << "\n";
		return -1;
	}


	int ret = SetupInstallFromInfSection(
		NULL, //owner
		inf, //inf handle
		L"DefaultInstall",
		SPINST_ALL, //flags
		NULL, //RelativeKeyRoot
		NULL, //SourceRootPath
		SP_COPY_NEWER_OR_SAME | SP_COPY_IN_USE_NEEDS_REBOOT, //CopyFlags
		NULL, //MsgHandler
		NULL, //Context
		NULL, //DeviceInfoSet
		NULL //DeviceInfoData
	);

	if (!ret) {
		std::cout << "SetupInstallFromInfSection failed, err " << GetLastError() << "\n";
		return -1;
	}

	SetupCloseInfFile(inf);*/
}


#define ZFSIOCTL_TYPE 40000

void send_zfs_ioc_unregister_fs() {
	
	fprintf(stderr, "send_zfs_ioc_unregister_fs()");

	HANDLE g_fd = CreateFile(L"\\\\.\\ZFS", GENERIC_READ | GENERIC_WRITE,
		0, NULL, OPEN_EXISTING, 0, NULL);

	DWORD bytesReturned;

	BOOL ret = DeviceIoControl(
		g_fd,
		CTL_CODE(ZFSIOCTL_TYPE, 0x8E2, METHOD_NEITHER, FILE_ANY_ACCESS),
		NULL,
		0,
		NULL,
		0,
		&bytesReturned,
		NULL
		);

	CloseHandle(g_fd);

}
