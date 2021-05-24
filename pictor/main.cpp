/* Image viewer and converter. */

#include <windows.h>
#include <fstream>
#include <string>
#include <vector>


HBITMAP gBitmap = NULL;

struct RGB
{
	unsigned char r, g, b;
};

struct ImagePPM
{
	UINT width;
	UINT height;
	BYTE* data;
};

int fpeek(FILE *stream)
{
	int c = fgetc(stream);
	ungetc(c, stream);
	return c;
}

void eat_comment(FILE *f)
{
	char linebuf[1024], ppp;
	while (ppp = fpeek(f), ppp == '\n' || ppp == '\r') fgetc(f);
	if (ppp == '#') fgets(linebuf, 1023, f);
}

bool load_ppm(ImagePPM& img, const std::string& name)
{
	FILE *file = fopen(name.c_str(), "rb");
	if (!file) return false;

	// get type of file
	eat_comment(file);
	char buf[1024];
	fscanf(file, "%s", buf);
	int mode = buf[1] - '0';

	// get width
	eat_comment(file);
	fscanf(file, "%d", &img.width);

	// get height
	eat_comment(file);
	fscanf(file, "%d", &img.height);

	// get bits
	eat_comment(file);
	int bits = 0;
	fscanf(file, "%d", &bits);

	// load image data
	img.data = (BYTE*)malloc(img.width * img.height);

	switch (mode)
	{
	case 1: // monochrome (ASCII)
		for (size_t i = 0; i < img.width * img.height; i++)
		{
			BYTE v;
			fscanf(file, "%d", &v);
			if (v > 0) v = 255;
			img.data[i] = v;
		}
		break;

	case 2: // grayscale (ASCII)
		for (size_t i = 0; i < img.width * img.height; i++)
		{
			BYTE v;
			fscanf(file, "%d", &v);
			v = (v / (float)bits) * 255;
			img.data[i] = v;
		}
		break;

	case 3: // RGB (ASCII)
		for (size_t i = 0; i < img.width * img.height; i+=3)
		{
			BYTE r, g, b;
			fscanf(file, " %c", &r);
			fscanf(file, " %c", &g);
			fscanf(file, " %c", &b);
			img.data[i+0] = r;
			img.data[i+1] = g;
			img.data[i+2] = b;
		}
		break;

	case 4: // monochrome (binary)
		break;

	case 5: // grayscale (binary)
		break;

	case 6: // RGB (binary)
		fgetc(file);
		fread((BYTE*)&img.data[0], 1, img.width * img.height * 3, file);
		break;
	}

	// close file
	fclose(file);
	return true;
}


LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_CLOSE:
		{
			DestroyWindow(hwnd);
			break;
		}

		case WM_DESTROY:
		{
			PostQuitMessage(0);
			break;
		}

		case WM_PAINT:
		{
			BITMAP bm;
			PAINTSTRUCT ps;

			HDC hdc = BeginPaint(hwnd, &ps);
			HDC hdcMem = CreateCompatibleDC(hdc);
			HGDIOBJ hbmOld = SelectObject(hdcMem, gBitmap);

			GetObject(gBitmap, sizeof(bm), &bm);
			BitBlt(hdc, 0, 0, bm.bmWidth, bm.bmHeight, hdcMem, 0, 0, SRCCOPY);
			
			SelectObject(hdcMem, hbmOld);
			DeleteDC(hdcMem);
			
			EndPaint(hwnd, &ps);
			break;
		}

		case WM_KEYDOWN:
		{
			if (wParam == VK_ESCAPE)
				DestroyWindow(hwnd);
			break;
		}
		

		default:
			return DefWindowProc(hwnd, msg, wParam, lParam);
	}

	return 0;
}


int __stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	ImagePPM ppm;
	if (!load_ppm(ppm, "tree_1.ppm")) return 0;

	UINT width = ppm.width;
	UINT height = ppm.height;

	WNDCLASSEX wc;
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = 0;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = "WindowClass";
	wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	if (!RegisterClassEx(&wc))
	{
		MessageBox(NULL, "Window registration failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

	HWND hwnd = CreateWindowEx
	(
		WS_EX_CLIENTEDGE,
		"WindowClass",
		"",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		width,
		height,
		NULL,
		NULL,
		hInstance,
		NULL
	);

	if (hwnd == NULL)
	{
		MessageBox(NULL, "Window creation failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

	gBitmap = CreateBitmap(width, height, 1, sizeof(DWORD) * 8, (void*)&ppm.data[0]);
	
	ShowWindow(hwnd, nCmdShow);
	UpdateWindow(hwnd);


	MSG msg = { 0 };
	while (msg.message != WM_QUIT)
	{
		while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	
	DeleteObject(gBitmap);

	return msg.wParam;
}

