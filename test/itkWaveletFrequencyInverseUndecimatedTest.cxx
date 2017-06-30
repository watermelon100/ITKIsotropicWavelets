/*=========================================================================
 *
 *  Copyright Insight Software Consortium
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/

#include "itkImage.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkWaveletFrequencyForwardUndecimated.h"
#include "itkWaveletFrequencyInverseUndecimated.h"
#include "itkWaveletFrequencyFilterBankGenerator.h"
#include "itkHeldIsotropicWavelet.h"
#include "itkVowIsotropicWavelet.h"
#include "itkSimoncelliIsotropicWavelet.h"
#include "itkShannonIsotropicWavelet.h"
#include "itkForwardFFTImageFilter.h"
#include "itkInverseFFTImageFilter.h"
#include "itkTestingMacros.h"

#include <memory>
#include <string>
#include <cmath>

#ifdef ITK_VISUALIZE_TESTS
#include "itkComplexToRealImageFilter.h"
#include "itkNumberToString.h"
#include "itkViewImage.h"
#endif

template< unsigned int VDimension, typename TWaveletFunction >
int
runWaveletFrequencyInverseUndecimatedTest( const std::string& inputImage,
  const std::string& outputImage,
  const unsigned int& inputLevels,
  const unsigned int& inputBands,
  bool inputUseWaveletFilterBankPyramid)
{
  bool testPassed = true;
  const unsigned int Dimension = VDimension;

  typedef float                              PixelType;
  typedef itk::Image< PixelType, Dimension > ImageType;
  typedef itk::ImageFileReader< ImageType >  ReaderType;

  typename ReaderType::Pointer reader = ReaderType::New();
  reader->SetFileName( inputImage );
  reader->Update();
  reader->UpdateLargestPossibleRegion();

  // Perform FFT on input image.
  typedef itk::ForwardFFTImageFilter< ImageType > FFTFilterType;
  typename FFTFilterType::Pointer fftFilter = FFTFilterType::New();
  fftFilter->SetInput( reader->GetOutput() );

  typedef typename FFTFilterType::OutputImageType ComplexImageType;

  // Set the WaveletFunctionType and the WaveletFilterBank
  typedef TWaveletFunction WaveletFunctionType;
  typedef itk::WaveletFrequencyFilterBankGenerator< ComplexImageType, WaveletFunctionType >
    WaveletFilterBankType;
  typedef itk::WaveletFrequencyForwardUndecimated< ComplexImageType, ComplexImageType, WaveletFilterBankType >
    ForwardWaveletType;

  typename ForwardWaveletType::Pointer forwardWavelet = ForwardWaveletType::New();

  forwardWavelet->SetHighPassSubBands( inputBands );
  forwardWavelet->SetLevels( inputLevels );
  forwardWavelet->SetInput( fftFilter->GetOutput() );
  forwardWavelet->StoreWaveletFilterBankPyramidOn();
  forwardWavelet->Update();

  unsigned int noutputs = forwardWavelet->GetNumberOfOutputs();

  std::cout << "Number of inputs: " << noutputs << std::endl;
  for ( unsigned int i = 0; i < noutputs; ++i )
    {
    std::cout << "Input number: " << i << std::endl;
    std::cout << "Region: " << forwardWavelet->GetOutput(i)->GetLargestPossibleRegion() << std::endl;
    std::cout << "Spacing: " << forwardWavelet->GetOutput(i)->GetSpacing() << std::endl;
    }

  // Inverse Wavelet Transform
  typedef itk::WaveletFrequencyInverseUndecimated< ComplexImageType, ComplexImageType, WaveletFilterBankType >
    InverseWaveletType;
  typename InverseWaveletType::Pointer inverseWavelet = InverseWaveletType::New();

  inverseWavelet->SetHighPassSubBands( inputBands );
  inverseWavelet->SetLevels( inputLevels );
  inverseWavelet->SetInputs( forwardWavelet->GetOutputs() );
  inverseWavelet->SetUseWaveletFilterBankPyramid( inputUseWaveletFilterBankPyramid );
  inverseWavelet->SetWaveletFilterBankPyramid( forwardWavelet->GetWaveletFilterBankPyramid() );
  inverseWavelet->DebugOn();
  inverseWavelet->Update();

  // Check Metadata: Spacing, Origin
  typename ComplexImageType::SpacingType outputSpacing =
    inverseWavelet->GetOutput()->GetSpacing();
  typename ComplexImageType::SpacingType expectedSpacing;
  expectedSpacing.Fill(1.0);
  typename ComplexImageType::PointType outputOrigin =
    inverseWavelet->GetOutput()->GetOrigin();
  typename ComplexImageType::PointType expectedOrigin;
  expectedOrigin.Fill(0.0);
  typename ComplexImageType::SizeType outputSize =
    inverseWavelet->GetOutput()->GetLargestPossibleRegion().GetSize();
  typename ComplexImageType::SizeType expectedSize =
    fftFilter->GetOutput()->GetLargestPossibleRegion().GetSize();

  if ( outputSpacing != expectedSpacing )
    {
    std::cout << "outputSpacing is wrong: " << outputSpacing
              << " expectedSpacing: " << expectedSpacing
              << std::endl;
    testPassed = false;
    }
  if ( outputOrigin != expectedOrigin )
    {
    std::cout << "outputOrigin is wrong: " << outputOrigin
              << " expectedOrigin: " << expectedOrigin
              << std::endl;
    testPassed = false;
    }
  if ( outputSize != expectedSize )
    {
    std::cout << "outputSize is wrong: " << outputSize
              << " expectedSize: " << expectedSize
              << std::endl;
    testPassed = false;
    }

  typedef itk::InverseFFTImageFilter< ComplexImageType, ImageType > InverseFFTFilterType;
  typename InverseFFTFilterType::Pointer inverseFFT = InverseFFTFilterType::New();
  inverseFFT->SetInput( inverseWavelet->GetOutput() );
  inverseFFT->Update();

  // Write output image
  typedef itk::ImageFileWriter< ImageType > WriterType;
  typename WriterType::Pointer writer = WriterType::New();
  writer->SetFileName( outputImage );
  writer->SetInput( inverseFFT->GetOutput() );

  TRY_EXPECT_NO_EXCEPTION( writer->Update() );

#ifdef ITK_VISUALIZE_TESTS
  itk::Testing::ViewImage( reader->GetOutput(), "Original" );
  itk::Testing::ViewImage( inverseFFT->GetOutput(), "InverseWavelet" );
#endif

  // TODO move it from here to Forward test.
#ifdef ITK_VISUALIZE_TESTS
  std::vector< typename ComplexImageType::Pointer > waveletFilterBankPyramid =
    forwardWavelet->GetWaveletFilterBankPyramid();
  typedef itk::ComplexToRealImageFilter< ComplexImageType, ImageType > ComplexToRealFilterType;
  typename ComplexToRealFilterType::Pointer complexToRealFilter = ComplexToRealFilterType::New();

  itk::NumberToString< unsigned int > n2s;
  std::cout << "Size FilterBankPyramid:" << waveletFilterBankPyramid.size() << std::endl;
  for ( unsigned int i = 0; i < waveletFilterBankPyramid.size(); ++i )
    {
    complexToRealFilter->SetInput( waveletFilterBankPyramid[i]);
    complexToRealFilter->UpdateLargestPossibleRegion();
    itk::Testing::ViewImage( complexToRealFilter->GetOutput(), "FilterBankPyramid #" + n2s(i) );
    }
#endif

  if ( testPassed )
    {
    return EXIT_SUCCESS;
    }
  else
    {
    return EXIT_FAILURE;
    }
}

int
itkWaveletFrequencyInverseUndecimatedTest(int argc, char *argv[])
{
  if ( argc != 8 )
    {
    std::cerr << "Usage: " << argv[0]
              << " inputImage outputImage inputLevels inputBands waveletFunction reuseFilterBankPyramid|noFilterBankPyramid dimension" << std::endl;
    return EXIT_FAILURE;
    }

  const std::string inputImage  = argv[1];
  const std::string outputImage = argv[2];
  const unsigned int inputLevels = atoi( argv[3] );
  const unsigned int inputBands  = atoi( argv[4] );
  const std::string waveletFunction = argv[5];
  const std::string inputUseWaveletFilterBankPyramid = argv[6];
  bool useWaveletFilterBankPyramid;
  if (inputUseWaveletFilterBankPyramid == "reuseFilterBankPyramid")
    {
    useWaveletFilterBankPyramid = true;
    }
  else if (inputUseWaveletFilterBankPyramid == "noFilterBankPyramid")
    {
    useWaveletFilterBankPyramid = false;
    }

  unsigned int dimension = atoi( argv[7] );

  // const unsigned int ImageDimension = 3;
  // typedef double                                          PixelType;
  // typedef std::complex< PixelType >                       ComplexPixelType;
  // typedef itk::Point< PixelType, ImageDimension >         PointType;
  // typedef itk::Image< ComplexPixelType, ImageDimension >  ComplexImageType;

  // // Exercise basic object methods
  // // Done outside the helper function in the test because GCC is limited
  // // when calling overloaded base class functions.
  // typedef itk::HeldIsotropicWavelet< PixelType, ImageDimension, PointType >
  //   HeldIsotropicWaveletType;
  // typedef itk::VowIsotropicWavelet< PixelType, ImageDimension, PointType >
  //   VowIsotropicWaveletType;
  // typedef itk::SimoncelliIsotropicWavelet< PixelType, ImageDimension, PointType >
  //   SimoncelliIsotropicWaveletType;
  // typedef itk::ShannonIsotropicWavelet< PixelType, ImageDimension, PointType >
  //   ShannonIsotropicWaveletType;
  //
  // HeldIsotropicWaveletType::Pointer heldIsotropicWavelet =
  //   HeldIsotropicWaveletType::New();
  // EXERCISE_BASIC_OBJECT_METHODS( heldIsotropicWavelet, HeldIsotropicWavelet,
  //   IsotropicWaveletFrequencyFunction );
  //
  // VowIsotropicWaveletType::Pointer vowIsotropicWavelet =
  //   VowIsotropicWaveletType::New();
  // EXERCISE_BASIC_OBJECT_METHODS( vowIsotropicWavelet, VowIsotropicWavelet,
  //   IsotropicWaveletFrequencyFunction );
  //
  // SimoncelliIsotropicWaveletType::Pointer simoncellidIsotropicWavelet =
  //   SimoncelliIsotropicWaveletType::New();
  // EXERCISE_BASIC_OBJECT_METHODS( simoncellidIsotropicWavelet, SimoncelliIsotropicWavelet,
  //   IsotropicWaveletFrequencyFunction );
  //
  // ShannonIsotropicWaveletType::Pointer shannonIsotropicWavelet = ShannonIsotropicWaveletType::New();
  // EXERCISE_BASIC_OBJECT_METHODS( shannonIsotropicWavelet, ShannonIsotropicWavelet,
  //   IsotropicWaveletFrequencyFunction );
  //
  //
  typedef itk::HeldIsotropicWavelet< >       HeldWavelet;
  typedef itk::VowIsotropicWavelet< >        VowWavelet;
  typedef itk::SimoncelliIsotropicWavelet< > SimoncelliWavelet;
  typedef itk::ShannonIsotropicWavelet< >    ShannonWavelet;
  //
  //
  // typedef itk::WaveletFrequencyFilterBankGenerator< ComplexImageType, HeldWavelet >
  //   HeldWaveletFilterBankType;
  // typedef itk::WaveletFrequencyFilterBankGenerator< ComplexImageType, VowWavelet >
  //   VowWaveletFilterBankType;
  // typedef itk::WaveletFrequencyFilterBankGenerator< ComplexImageType, SimoncelliWavelet >
  //   SimoncelliWaveletFilterBankType;
  // typedef itk::WaveletFrequencyFilterBankGenerator< ComplexImageType, ShannonWavelet >
  //   ShannonWaveletFilterBankType;
  //
  // typedef itk::WaveletFrequencyInverseUndecimated< ComplexImageType, ComplexImageType, HeldWaveletFilterBankType >
  //   HeldInverseWaveletType;
  // HeldInverseWaveletType::Pointer heldInverseWavelet = HeldInverseWaveletType::New();
  // EXERCISE_BASIC_OBJECT_METHODS( heldInverseWavelet, WaveletFrequencyInverseUndecimated,
  //   ImageToImageFilter );
  //
  // typedef itk::WaveletFrequencyInverseUndecimated< ComplexImageType, ComplexImageType, VowWaveletFilterBankType >
  //   VowInverseWaveletType;
  // VowInverseWaveletType::Pointer vowInverseWavelet = VowInverseWaveletType::New();
  // EXERCISE_BASIC_OBJECT_METHODS( vowInverseWavelet, WaveletFrequencyInverseUndecimated,
  //   ImageToImageFilter );
  //
  // typedef itk::WaveletFrequencyInverseUndecimated< ComplexImageType, ComplexImageType, SimoncelliWaveletFilterBankType >
  //   SimoncelliInverseWaveletType;
  // SimoncelliInverseWaveletType::Pointer simoncelliInverseWavelet = SimoncelliInverseWaveletType::New();
  // EXERCISE_BASIC_OBJECT_METHODS( simoncelliInverseWavelet, WaveletFrequencyInverseUndecimated,
  //   ImageToImageFilter );
  //
  // typedef itk::WaveletFrequencyInverseUndecimated< ComplexImageType, ComplexImageType, ShannonWaveletFilterBankType >
  //   ShannonInverseWaveletType;
  // ShannonInverseWaveletType::Pointer shannonInverseWavelet = ShannonInverseWaveletType::New();
  // EXERCISE_BASIC_OBJECT_METHODS( shannonInverseWavelet, WaveletFrequencyInverseUndecimated,
  //   ImageToImageFilter );

  if ( dimension == 2 )
    {
    if ( waveletFunction == "Held" )
      {
      return runWaveletFrequencyInverseUndecimatedTest< 2, HeldWavelet >( inputImage, outputImage, inputLevels, inputBands, useWaveletFilterBankPyramid );
      }
    else if ( waveletFunction == "Vow" )
      {
      return runWaveletFrequencyInverseUndecimatedTest< 2, VowWavelet >( inputImage, outputImage, inputLevels, inputBands, useWaveletFilterBankPyramid );
      }
    else if ( waveletFunction == "Simoncelli" )
      {
      return runWaveletFrequencyInverseUndecimatedTest< 2, SimoncelliWavelet >( inputImage, outputImage, inputLevels, inputBands, useWaveletFilterBankPyramid );
      }
    else if ( waveletFunction == "Shannon" )
      {
      return runWaveletFrequencyInverseUndecimatedTest< 2, ShannonWavelet >( inputImage, outputImage, inputLevels, inputBands, useWaveletFilterBankPyramid );
      }
    else
      {
      std::cerr << "Test failed!" << std::endl;
      std::cerr << argv[5] << " wavelet type not supported." << std::endl;
      return EXIT_FAILURE;
      }
    }
  else if ( dimension == 3 )
    {
    if ( waveletFunction == "Held" )
      {
      return runWaveletFrequencyInverseUndecimatedTest< 3, HeldWavelet >( inputImage, outputImage, inputLevels, inputBands, useWaveletFilterBankPyramid );
      }
    else if ( waveletFunction == "Vow" )
      {
      return runWaveletFrequencyInverseUndecimatedTest< 3, VowWavelet >( inputImage, outputImage, inputLevels, inputBands, useWaveletFilterBankPyramid );
      }
    else if ( waveletFunction == "Simoncelli" )
      {
      return runWaveletFrequencyInverseUndecimatedTest< 3, SimoncelliWavelet >( inputImage, outputImage, inputLevels, inputBands, useWaveletFilterBankPyramid );
      }
    else if ( waveletFunction == "Shannon" )
      {
      return runWaveletFrequencyInverseUndecimatedTest< 3, ShannonWavelet >( inputImage, outputImage, inputLevels, inputBands, useWaveletFilterBankPyramid );
      }
    else
      {
      std::cerr << "Test failed!" << std::endl;
      std::cerr << argv[5] << " wavelet type not supported." << std::endl;
      return EXIT_FAILURE;
      }
    }
  else
    {
    std::cerr << "Test failed!" << std::endl;
    std::cerr << "Error: only 2 or 3 dimensions allowed, " << dimension << " selected." << std::endl;
    return EXIT_FAILURE;
    }
}