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
#ifndef itkHeldIsotropicWavelet_h
#define itkHeldIsotropicWavelet_h

#include <itkIsotropicWaveletFrequencyFunction.h>

namespace itk
{
/** \class HeldIsotropicWavelet
 * \brief Wavelet based on paper Steerable Wavelet Frames Based on the Held
 * Transform (Held et al 2010).
 *
 * Implement function in frequency space.
 *
 * h(w) = cos(2*pi*q(|w|)) for w in (1/8, 1/4]
 * h(w) = sin(2*pi*q(|w/2|)) for w in (1/4, 1/2]
 * h(w) = 0 elsewhere.
 *
 * Where q(t) is a m grade polynomial (m can be chosen) which elements are
 * calculated so the wavelet has desirable properties.
 * ie, tight frame, Held Paritition of Unity, etc. (see paper for more info)
 *
 * \ingroup SpatialFunctions
 * \ingroup IsotropicWavelets
 */
template< typename TFunctionValue = double,
          unsigned int VImageDimension = 3,
          typename TInput = Point< SpacePrecisionType, VImageDimension > >
class HeldIsotropicWavelet:
  public IsotropicWaveletFrequencyFunction< TFunctionValue, VImageDimension, TInput >
{
public:
  /** Standard class typedefs. */
  typedef HeldIsotropicWavelet                                        Self;
  typedef IsotropicWaveletFrequencyFunction< TFunctionValue, VImageDimension, TInput > Superclass;
  typedef SmartPointer< Self >                                Pointer;
  typedef SmartPointer< const Self >                          ConstPointer;

  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** Run-time type information (and related methods). */
  itkTypeMacro(HeldIsotropicWavelet, SpatialFunction);

  /** Input type for the function. */
  typedef typename Superclass::InputType InputType;

  /** FunctionValue type for the function. */
  typedef typename Superclass::FunctionValueType FunctionValueType;

  /** Type used to store gaussian parameters. */
  typedef FixedArray< double, VImageDimension > ArrayType;

  /** Evaluate the function */
  FunctionValueType EvaluateMagnitude(const FunctionValueType& freq_norm_in_hz) const ITK_OVERRIDE;

  /**** Forward/Analysis ***/
  /** Evaluate the low filter response. */
  FunctionValueType EvaluateForwardLowPassFilter(const FunctionValueType& freq_norm_in_hz) const ITK_OVERRIDE;
  /** Evaluate the highfilter response. */
  FunctionValueType EvaluateForwardHighPassFilter(const FunctionValueType& freq_norm_in_hz) const ITK_OVERRIDE;
  /** Evaluate the sub-band response.
   * j = 0 evaluates LowFilter, j=m_SubBand evaluates HighFilter */
  FunctionValueType EvaluateForwardSubBand( const FunctionValueType& freq_norm_in_hz,
      unsigned int j) const ITK_OVERRIDE;
  /**** Inverse/Synthesis ***/
  /** Evaluate the low filter response. */
  FunctionValueType EvaluateInverseLowPassFilter(const FunctionValueType& freq_norm_in_hz) const ITK_OVERRIDE;
  /** Evaluate the highfilter response. */
  FunctionValueType EvaluateInverseHighPassFilter(const FunctionValueType& freq_norm_in_hz) const ITK_OVERRIDE;
  /** Evaluate the sub-band response.
   * j = 0 evaluates LowFilter, j=m_SubBand evaluates HighFilter */
  FunctionValueType EvaluateInverseSubBand( const FunctionValueType& freq_norm_in_hz,
      unsigned int j) const ITK_OVERRIDE;

  /** Gets and sets parameters */
  itkSetMacro(PolynomialOrder, unsigned int);
  itkGetConstMacro(PolynomialOrder, unsigned int);

  FunctionValueType ComputePolynom(
      const FunctionValueType & freq_norm_in_hz,
      const unsigned int & order) const;

protected:
  HeldIsotropicWavelet();
  virtual ~HeldIsotropicWavelet();
  void PrintSelf(std::ostream & os, Indent indent) const ITK_OVERRIDE;

private:
  HeldIsotropicWavelet(const Self &) ITK_DELETE_FUNCTION;
  void operator=(const Self &) ITK_DELETE_FUNCTION;

  /** The order of the polynom. */
  unsigned int m_PolynomialOrder;

};
} // end namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#include "itkHeldIsotropicWavelet.hxx"
#endif

#endif