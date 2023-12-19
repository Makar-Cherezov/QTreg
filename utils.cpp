#include "utils.h"

std::vector<std::string> GetFilesInDir(std::filesystem::path directoryPath)
{
  std::vector<std::string> fileNames;
  for (const auto & entry :
       std::filesystem::directory_iterator(directoryPath))
  {
    if (std::filesystem::is_regular_file(entry) &&
        (entry.path().extension() == ".tif" ||
         entry.path().extension() == ".tiff"))
    {
      fileNames.push_back(entry.path().string());
    }
  }
  return fileNames;
}

void fillFileNames(std::vector<std::string>* srcFileNames, std::vector<std::string>* refFileNames, Ui::MainWindow* ui)
{
        QDir srcDir, refDir;
        srcDir = QFileInfo(ui->srcPathLabel->text()).absoluteFilePath();
        srcDir.setFilter(QDir::Files | QDir::Readable);
        refDir = QFileInfo(ui->refPathLabel->text()).absoluteFilePath();
        refDir.setFilter(QDir::Files | QDir::Readable);
        for ( QString str : srcDir.entryList()) {
          srcFileNames->push_back(str.toStdString());
        }
        for ( QString str : refDir.entryList()) {
          refFileNames->push_back(str.toStdString());
        }
}

void PrintSlice(itk::Image<float, 3>::Pointer fixedImage)
{
    std::ofstream pixelFile("pixel_values.txt");
    for (int i = 120; i <= 190; ++i)
    {
        for (int j = 120; j <= 190; ++j)
        {
            for (int k = 120; k <= 190; ++k)
            {
                itk::Index<3> pixelIndex;
                pixelIndex[0] = i;
                pixelIndex[1] = j;
                pixelIndex[3] = k;
                pixelFile << fixedImage->GetPixel(pixelIndex) << ' ';
            }
            pixelFile << std::endl;
        }
        pixelFile << std::endl;
    }
    pixelFile.close();
}
