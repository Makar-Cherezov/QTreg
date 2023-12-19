#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "itkImageRegistrationMethodv4.h"
#include "itkMeanSquaresImageToImageMetricv4.h"
#include <filesystem>
#include "itkImageSeriesReader.h"
#include "itkTIFFImageIO.h"
#include "itkVersorRigid3DTransform.h"
#include "itkCenteredTransformInitializer.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkResampleImageFilter.h"
#include "itkCastImageFilter.h"
#include "itkSubtractImageFilter.h"
#include "itkRescaleIntensityImageFilter.h"
#include "itkExtractImageFilter.h"
#include <QMessageBox>
#include <QDebug>
#include <QFileDialog>
#include <fstream>
#include "commanditerationupdate.h"
#include "utils.h"



MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_pushButton_clicked()
{


    QMessageBox msgBox;

    constexpr unsigned int Dimension = 3;
    using PixelType = float;
    using FixedImageType = itk::Image<PixelType, Dimension>;
    using MovingImageType = itk::Image<PixelType, Dimension>;
    using TransformType = itk::VersorRigid3DTransform<double>;
    using OptimizerType = itk::RegularStepGradientDescentOptimizerv4<double>;
    using MetricType =
      itk::MeanSquaresImageToImageMetricv4<FixedImageType, MovingImageType>;
    using RegistrationType = itk::
      ImageRegistrationMethodv4<FixedImageType, MovingImageType, TransformType>;

    MetricType::Pointer       metric = MetricType::New();
    OptimizerType::Pointer    optimizer = OptimizerType::New();
    RegistrationType::Pointer registration = RegistrationType::New();

    registration->SetMetric(metric);
    registration->SetOptimizer(optimizer);


    TransformType::Pointer initialTransform = TransformType::New();


    using FixedImageReaderType = itk::ImageSeriesReader<FixedImageType>;
    using MovingImageReaderType = itk::ImageSeriesReader<MovingImageType>;
    FixedImageReaderType::Pointer fixedImageReader =
      FixedImageReaderType::New();
    MovingImageReaderType::Pointer movingImageReader =
      MovingImageReaderType::New();

    std::vector<std::string> srcFileNames, refFileNames;
    fillFileNames(&srcFileNames, &refFileNames, ui);
    std::cout << srcFileNames[0] << std::endl << refFileNames[1];
    std::cerr << "Filenames read" << std::endl;
    fixedImageReader->SetImageIO(itk::TIFFImageIO::New());
    movingImageReader->SetImageIO(itk::TIFFImageIO::New());

    fixedImageReader->SetFileNames(srcFileNames);
    movingImageReader->SetFileNames(refFileNames);


    registration->SetFixedImage(fixedImageReader->GetOutput());
    registration->SetMovingImage(movingImageReader->GetOutput());
    std::cerr << "Images set" << std::endl;

    PrintSlice(fixedImageReader->GetOutput());
    std::cerr << "Slice printed" << std::endl;

    using TransformInitializerType =
      itk::CenteredTransformInitializer<TransformType,
                                        FixedImageType,
                                        MovingImageType>;
    TransformInitializerType::Pointer initializer =
      TransformInitializerType::New();

    initializer->SetTransform(initialTransform);
    initializer->SetFixedImage(fixedImageReader->GetOutput());
    initializer->SetMovingImage(movingImageReader->GetOutput());

    initializer->MomentsOn();

    initializer->InitializeTransform();

    using VersorType = TransformType::VersorType;
    using VectorType = VersorType::VectorType;
    VersorType rotation;
    VectorType axis;
    axis[0] = 0.0;
    axis[1] = 0.0;
    axis[2] = 1.0;
    constexpr double angle = 0;
    rotation.Set(axis, angle);
    initialTransform->SetRotation(rotation);

    registration->SetInitialTransform(initialTransform);
    std::cerr << "Initial transform set" << std::endl;
    using OptimizerScalesType = OptimizerType::ScalesType;
    OptimizerScalesType optimizerScales(
      initialTransform->GetNumberOfParameters());
    const double translationScale = 1.0 / 1000.0;
    optimizerScales[0] = 1.0;
    optimizerScales[1] = 1.0;
    optimizerScales[2] = 1.0;
    optimizerScales[3] = translationScale;
    optimizerScales[4] = translationScale;
    optimizerScales[5] = translationScale;
    optimizer->SetScales(optimizerScales);
    optimizer->SetNumberOfIterations(200);
    optimizer->SetLearningRate(0.2);
    optimizer->SetMinimumStepLength(0.001);
    optimizer->SetReturnBestParametersAndValue(true);
    std::cerr << "Optimizer set" << std::endl;

    CommandIterationUpdate::Pointer observer = CommandIterationUpdate::New();
    optimizer->AddObserver(itk::IterationEvent(), observer);

    constexpr unsigned int numberOfLevels = 1;

    RegistrationType::ShrinkFactorsArrayType shrinkFactorsPerLevel;
    shrinkFactorsPerLevel.SetSize(1);
    shrinkFactorsPerLevel[0] = 1;

    RegistrationType::SmoothingSigmasArrayType smoothingSigmasPerLevel;
    smoothingSigmasPerLevel.SetSize(1);
    smoothingSigmasPerLevel[0] = 0;

    registration->SetNumberOfLevels(numberOfLevels);
    registration->SetSmoothingSigmasPerLevel(smoothingSigmasPerLevel);
    registration->SetShrinkFactorsPerLevel(shrinkFactorsPerLevel);
    std::cerr << "Smoothing set" << std::endl;
    try
    {
      std::cerr << "Updating registration" << std::endl;
      registration->Update();
      std::cerr << registration->GetOptimizer()->GetStopConditionDescription() << std::endl;
    }
    catch (const itk::ExceptionObject & err)
    {
        std::cerr << err.GetDescription() << std::endl;
    }

    const TransformType::ParametersType finalParameters =
      registration->GetOutput()->Get()->GetParameters();
    std::cerr << "Registration parameters" << std::endl;
    const double       versorX = finalParameters[0];
    const double       versorY = finalParameters[1];
    const double       versorZ = finalParameters[2];
    const double       finalTranslationX = finalParameters[3];
    const double       finalTranslationY = finalParameters[4];
    const double       finalTranslationZ = finalParameters[5];
    const unsigned int numberOfIterations = optimizer->GetCurrentIteration();
    const double       bestValue = optimizer->GetValue();





    TransformType::Pointer finalTransform = TransformType::New();

    finalTransform->SetFixedParameters(
      registration->GetOutput()->Get()->GetFixedParameters());
    finalTransform->SetParameters(finalParameters);

    TransformType::MatrixType matrix = finalTransform->GetMatrix();
    TransformType::OffsetType offset = finalTransform->GetOffset();
//    std::cout << "Matrix = " << std::endl << matrix << std::endl;
//    std::cout << "Offset = " << std::endl << offset << std::endl;

    using ResampleFilterType =
      itk::ResampleImageFilter<MovingImageType, FixedImageType>;

    ResampleFilterType::Pointer resampler = ResampleFilterType::New();

    resampler->SetTransform(finalTransform);
    resampler->SetInput(movingImageReader->GetOutput());

    FixedImageType::Pointer fixedImage = fixedImageReader->GetOutput();

    resampler->SetSize(fixedImage->GetLargestPossibleRegion().GetSize());
    resampler->SetOutputOrigin(fixedImage->GetOrigin());
    resampler->SetOutputSpacing(fixedImage->GetSpacing());
    resampler->SetOutputDirection(fixedImage->GetDirection());
    resampler->SetDefaultPixelValue(100);

    using OutputPixelType = unsigned char;
    using OutputImageType = itk::Image<OutputPixelType, Dimension>;
    using CastFilterType =
      itk::CastImageFilter<FixedImageType, OutputImageType>;
    using WriterType = itk::ImageFileWriter<OutputImageType>;

    WriterType::Pointer     writer = WriterType::New();
    CastFilterType::Pointer caster = CastFilterType::New();

    writer->SetFileName(ui->saveLabel->text().toStdString());

    caster->SetInput(resampler->GetOutput());
    writer->SetInput(caster->GetOutput());
    writer->Update();

    using DifferenceFilterType =
      itk::SubtractImageFilter<FixedImageType, FixedImageType, FixedImageType>;
    DifferenceFilterType::Pointer difference = DifferenceFilterType::New();

    using RescalerType =
      itk::RescaleIntensityImageFilter<FixedImageType, OutputImageType>;
    RescalerType::Pointer intensityRescaler = RescalerType::New();

    intensityRescaler->SetInput(difference->GetOutput());
    intensityRescaler->SetOutputMinimum(0);
    intensityRescaler->SetOutputMaximum(255);

    difference->SetInput1(fixedImageReader->GetOutput());
    difference->SetInput2(resampler->GetOutput());

    resampler->SetDefaultPixelValue(1);

    WriterType::Pointer writer2 = WriterType::New();
    writer2->SetInput(intensityRescaler->GetOutput());
    std::stringstream ss;
    ss << std::endl << std::endl
    << "Result = " << std::endl
    << " versor X      = " << versorX << std::endl
    << " versor Y      = " << versorY << std::endl
    << " versor Z      = " << versorZ << std::endl
    << " Translation X = " << finalTranslationX << std::endl
    << " Translation Y = " << finalTranslationY << std::endl
    << " Translation Z = " << finalTranslationZ << std::endl
    << " Iterations    = " << numberOfIterations << std::endl
    << " Metric value  = " << bestValue << std::endl;
    std::string formatted_string = ss.str();
    msgBox.setInformativeText(QString::fromStdString(formatted_string));


}


void MainWindow::on_openSrcDir_clicked()
{
    QString srcdir = QFileDialog::getExistingDirectory(this, "Выберите директорию исходника", QDir::homePath());
    ui->srcPathLabel->setText(srcdir);
}


void MainWindow::on_openRefDir_clicked()
{
    QDir refdir = QDir(ui->srcPathLabel->text());
    refdir.cdUp();
    QString refpath = QFileDialog::getExistingDirectory(this, "Выберите директорию после деформации", refdir.path());
    ui->refPathLabel->setText(refpath);
}


void MainWindow::on_OpenSaveDir_clicked()
{
    QDir savedir = QDir(ui->srcPathLabel->text());
    savedir.cdUp();
    QString savepath = QFileDialog::getSaveFileName(this, "Выберите директорию сохранения", savedir.path(), tr("MetaImage (*.mhd)"));
    ui->saveLabel->setText(savepath);
}




