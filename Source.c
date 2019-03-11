#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

struct Color
{
	unsigned char R, G, B;
};
struct Size
{
	int Dimension, Width, Height, Padding;
};
struct Bitmap
{
	struct Size size;
	struct Color* Map;
	char *Header;
};
void Initialize(char *adress, struct Bitmap *image)
{
	FILE *source;
	source = fopen(adress, "rb");
	if (source == NULL)
	{
		printf("Nu am gasit imaginea sursa din care citesc");
		return;
	}
	fseek(source, 2, SEEK_SET);
	fread(&image->size.Dimension, sizeof(unsigned int), 1, source);
	fseek(source, 18, SEEK_SET);
	fread(&image->size.Width, sizeof(unsigned int), 1, source);
	fread(&image->size.Height, sizeof(unsigned int), 1, source);
	int padding;
	if (image->size.Width % 4 != 0)
		padding = 4 - (3 * image->size.Width) % 4;
	else
		padding = 0;
	image->size.Padding = padding;
	image->Map = malloc(sizeof(struct Color)*image->size.Width*image->size.Height);
	fseek(source, 0, SEEK_SET);
	image->Header = malloc(54);
	fread(image->Header, 1, 54, source);
	int i,j;
	unsigned char c;
	fseek(source, 54, SEEK_SET);
	for (i = 0; i < image->size.Height; i++)
	{
		for (j = 0; j < image->size.Width; j++)
		{
			fread(&c, 1, 1, source);
				image->Map[i+j*(image->size.Height)].B = c;
			fread(&c, 1, 1, source);
				image->Map[i + j*(image->size.Height)].G = c;
			fread(&c, 1, 1, source);
				image->Map[i + j*(image->size.Height)].R = c;
		}
		unsigned char* rezidual=malloc(4);
		fread(rezidual, 1, image->size.Padding, source);
	}
	fclose(source);
}
void Write(char *adress, struct Bitmap *image)
{
	FILE *destination;
	destination = fopen(adress, "wb");
	if (destination == NULL)
	{
		printf("Nu am gasit imaginea sursa din care citesc");
		return;
	}
	int i, j;
	fwrite(image->Header, 1, 54, destination);
	for (i = 0; i < image->size.Height; i++)
	{
		for (j = 0; j < image->size.Width; j++)
		{
			fwrite(&image->Map[i + j*(image->size.Height)].B, 1, 1, destination);
			fwrite(&image->Map[i + j*(image->size.Height)].G, 1, 1, destination);
			fwrite(&image->Map[i + j*(image->size.Height)].R, 1, 1, destination);
		}
		unsigned char* rezidual = calloc(1,4);
		fwrite(rezidual, 1, image->size.Padding, destination);
	}
	fclose(destination);
}
void TurnToGray(struct Bitmap *bmp)
{
	int i;
	for (i = 0; i < bmp->size.Width*bmp->size.Height; i++)
	{
		bmp->Map[i].R = bmp->Map[i].G = bmp->Map[i].B = bmp->Map[i].R*0.299 + bmp->Map[i].G*0.587 + bmp->Map[i].B*0.114;
	}
}
struct Window
{
	int x, y;
};
struct OneMatch
{
	struct Window window;
	float Corelation;
	unsigned char Digit;
};
struct TemplateMatch
{
	struct OneMatch *Matches;
	int NrOfMatches;
};
float MediumIntensity(struct Bitmap *image, int x, int y)
{
	int i, j;
	float resoult = 0;
	for (i = x; i < x + 11; i++)
	{
		for (j = y; j < y + 15; j++)
		{
			resoult += image->Map[i + j * (image->size.Width)].R;
		}
	}
	resoult /= 11 * 15;
	return resoult;
}
float StandardIntensityDeviation(struct Bitmap *image, int x, int y)
{
	int i, j;
	float resoult = 0;
	float mediumIntensity = MediumIntensity(image, x, y);
	for (i = x; i < x + 11; i++)
	{
		for (j = y; j < y + 15; j++)
		{
			resoult += (image->Map[j + i * (image->size.Height)].R - mediumIntensity)*(image->Map[j + i * (image->size.Height)].R - mediumIntensity);
		}
	}
	resoult /= 11 * 15 - 1;
	resoult = sqrt(resoult);
	return resoult;
}
float Corelation(struct Bitmap *image, struct Bitmap *sablon, int x, int y)
{
	int i, j;
	float resoult = 0;
	float imageMediumIntensity = MediumIntensity(image, x, y);
	float sablonMediumIntensity = MediumIntensity(sablon, 0, 0);
	float imageStandardIntensityDeviation = StandardIntensityDeviation(image, x, y);
	float sablonStandardIntenistyDeviation = StandardIntensityDeviation(sablon, 0, 0);
	for (i = x; i < x + 11; i++)
	{
		for (j = y; j < y + 15; j++)
		{
			resoult += (image->Map[i + j * (image->size.Width)].R - imageMediumIntensity)*(sablon->Map[(i - x) + (j - y)* (sablon->size.Width)].R - sablonMediumIntensity);
		}
	}
	resoult /= 11 * 15 * (imageStandardIntensityDeviation*sablonStandardIntenistyDeviation);
	return resoult;
}
void TemplateMatching(struct TemplateMatch *f, struct Bitmap *image, struct Bitmap *sablon, float prag, unsigned char digit)
{
	TurnToGray(image);
	int x, y;
	for (x = 0; x < image->size.Width - 11; x++)
	{
		for (y = 0; y < image->size.Height - 15; y++)
		{
			float cor =  Corelation(image, sablon, x, y);
			if (cor >= prag)
			{
				f->NrOfMatches++;
				f->Matches = realloc(f->Matches, f->NrOfMatches * sizeof(struct OneMatch));
				f->Matches[f->NrOfMatches - 1].window.x = y;
				f->Matches[f->NrOfMatches - 1].window.y = x;
				f->Matches[f->NrOfMatches - 1].Corelation = cor;
				f->Matches[f->NrOfMatches - 1].Digit = digit;
			}
		}
	}
}
void DrawRectangle(struct Bitmap *image, struct Window window, struct Color color)
{
	int i, j;
	for (i = window.x; i < window.x + 15; i++)
	{
		image->Map[i + window.y * (image->size.Height)] = color;
		image->Map[i + (window.y + 10) * (image->size.Height)] = color;
	}
	for (j = window.y; j < window.y + 11; j++)
	{
		image->Map[window.x + j * (image->size.Height)] = color;
		image->Map[window.x + 14 + j * (image->size.Height)] = color;
	}
}
int CorelationsComparer(void *corelation1, void *corelation2)
{
	struct OneMatch *C1 = corelation1;
	struct OneMatch *C2 = corelation2;
	return C1->Corelation < C2->Corelation;
}
void SortDetections(struct TemplateMatch *matches)
{
	qsort(matches->Matches, matches->NrOfMatches, sizeof(struct OneMatch), CorelationsComparer);
}
int IsOver(struct Window *Important, struct Window *Unimportant)
{
	return (Unimportant->x - Important->x < 15 && Unimportant->x - Important->x > -15) && (Unimportant->y - Important->y < 11 && Unimportant->y - Important->y > -11);
}
struct TemplateMatch DeleteNonMaximResoults(struct TemplateMatch *toDelete)
{
	SortDetections(toDelete);
	int i, j;
	for (i = 0; i < toDelete->NrOfMatches; i++)
	{
		if (toDelete->Matches[i].window.x != -1)
		{
			for (j = i + 1; j < toDelete->NrOfMatches; j++)
			{
				if (IsOver(&(toDelete->Matches[i].window), &(toDelete->Matches[j].window)))
				{
					toDelete->Matches[j].window.x = -1;
				}
			}
		}
	}
	struct TemplateMatch resoult;
	resoult.Matches = malloc(sizeof(struct OneMatch));
	resoult.NrOfMatches = 0;
	int x;
	for (x = 0; x < toDelete->NrOfMatches; x++)
	{
		if (toDelete->Matches[x].window.x != -1)
		{
			resoult.NrOfMatches++;
			resoult.Matches = realloc(resoult.Matches, resoult.NrOfMatches * sizeof(struct OneMatch));
			resoult.Matches[resoult.NrOfMatches - 1].window = toDelete->Matches[x].window;
			resoult.Matches[resoult.NrOfMatches - 1].Digit = toDelete->Matches[x].Digit;
		}
	}
	return resoult;
}
void FinalFunction()
{
	char  *PatternsRecognitionFolder, *PatternsRecognition,**Digits;
	PatternsRecognitionFolder = malloc(200);
	PatternsRecognition = malloc(200);
	Digits = malloc(40);
	printf("Type the path of the folder containing all the necessary images(digit<i>.bmp, test.bmp): ");
	scanf("%s", PatternsRecognitionFolder);
	strcpy(PatternsRecognition, PatternsRecognitionFolder);
	strcat(PatternsRecognition, "test.bmp");
	unsigned char i;
	for (i = 0; i < 10; i++)
	{
		char *str = malloc(2);
		sprintf(str, "%d", i);
		Digits[i] = malloc(200);
		strcpy(Digits[i], PatternsRecognitionFolder);
		strcat(Digits[i], "digit");
		strcat(Digits[i], str);
		strcat(Digits[i], ".bmp");
	}
	struct TemplateMatch match;
	match.NrOfMatches = 0;
	match.Matches = malloc(sizeof(struct OneMatch));
	struct Bitmap PatternsRecognitionImage;
	Initialize(PatternsRecognition, &PatternsRecognitionImage);
	for (i = 0; i < 10; i++)
	{
		struct Bitmap CurrentDigit;
		Initialize(Digits[i], &CurrentDigit);
		TemplateMatching(&match, &PatternsRecognitionImage, &CurrentDigit, 0.5, i);
	}
	struct TemplateMatch FinalResoult = DeleteNonMaximResoults(&match);
	struct Color *ColorsForDigits = malloc(10 * sizeof(struct Color));
	ColorsForDigits[0].R = 255; ColorsForDigits[0].G = 0; ColorsForDigits[0].B = 0;
	ColorsForDigits[1].R = 255; ColorsForDigits[1].G = 255; ColorsForDigits[1].B = 0;
	ColorsForDigits[2].R = 0; ColorsForDigits[2].G = 255; ColorsForDigits[2].B = 0;
	ColorsForDigits[3].R = 0; ColorsForDigits[3].G = 255; ColorsForDigits[3].B = 255;
	ColorsForDigits[4].R = 255; ColorsForDigits[4].G = 0; ColorsForDigits[4].B = 255;
	ColorsForDigits[5].R = 0; ColorsForDigits[5].G = 0; ColorsForDigits[5].B = 255;
	ColorsForDigits[6].R = 192; ColorsForDigits[6].G = 192; ColorsForDigits[6].B = 192;
	ColorsForDigits[7].R = 255; ColorsForDigits[7].G = 140; ColorsForDigits[7].B = 0;
	ColorsForDigits[8].R = 128; ColorsForDigits[8].G = 0; ColorsForDigits[8].B = 128;
	ColorsForDigits[9].R = 128; ColorsForDigits[9].G = 0; ColorsForDigits[9].B = 0;
	int k;
	for (k = 0; k < FinalResoult.NrOfMatches; k++)
	{
		DrawRectangle(&PatternsRecognitionImage, FinalResoult.Matches[k].window, ColorsForDigits[FinalResoult.Matches[k].Digit]);
	}
	Write(PatternsRecognition, &PatternsRecognitionImage);
}
int main()
{
	FinalFunction();
}
