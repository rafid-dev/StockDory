//
// Copyright (c) 2023 MantaRay authors. See the list of authors for more details.
// Licensed under MIT.
//

#ifndef MANTARAY_PERSPECTIVENNUE_H
#define MANTARAY_PERSPECTIVENNUE_H

#include <array>
#include <cstdint>
#include <cassert>

#include "PerspectiveAccumulator.h"
#include "../SIMD.h"
#include "../AccumulatorOperation.h"
#include "../IO/BinaryFileStream.h"
#include "../IO/BinaryMemoryStream.h"
#include "../IO/MarlinflowStream.h"

namespace MantaRay
{

    /// \brief A Perspective-accounting Neural Network.
    /// \tparam T The internal input layer type of the network. Currently only int16_t is supported.
    /// \tparam OT The internal output layer type of the network. Currently only int32_t is supported.
    /// \tparam Activation The activation function to use.
    /// \tparam InputSize The size of the input layer.
    /// \tparam HiddenSize The size of the hidden layer.
    /// \tparam OutputSize The size of the output layer.
    /// \tparam AccumulatorStackSize The size of the accumulator stack.
    /// \tparam Scale The scale factor of the network.
    /// \tparam QuantizationFeature The quantization factor of the input layer.
    /// \tparam QuantizationOutput The quantization factor of the output layer.
    /// \details This class implements a perspective-accounting neural network. It is a feed-forward network with
    ///          one hidden layer.
    ///
    ///          The architecture is as follows: Concat((InputLayer -> Activation -> HiddenLayer)x2, CTM) -> OutputLayer
    ///          where CTM is the Color To Move.
    template<typename T, typename OT, typename Activation,
            uint16_t InputSize, uint16_t HiddenSize, uint16_t OutputSize,
            uint16_t AccumulatorStackSize, T Scale, T QuantizationFeature, T QuantizationOutput>
    class PerspectiveNetwork
    {

        // Support more types in the future, but currently disable all others.
        static_assert(std::is_same_v<T, int16_t> && std::is_same_v<OT, int32_t>,
                "This type is currently not supported.");

        // Support less sizes in the future, but currently disable.
        static_assert(InputSize > 32 && HiddenSize > 32, "This network size is currently not supported.");

        // The accumulator stack size should not be 0 or less.
        static_assert(AccumulatorStackSize > 0, "The accumulator stack size must at least be greater than zero.");

        // The constants used should make sense. Change this later if requested.
        static_assert(Scale > 0 && QuantizationFeature > 127 && QuantizationOutput > 31,
                "These scale and quantization constants don't seem right.");

        private:
            constexpr static uint16_t ColorStride = 64 * 6;
            constexpr static uint8_t  PieceStride = 64    ;

#ifdef __AVX512BW__
            alignas(64) std::array<T , InputSize * HiddenSize     > FeatureWeight;
            alignas(64) std::array<T , HiddenSize                 > FeatureBias  ;
            alignas(64) std::array<T , HiddenSize * 2 * OutputSize> OutputWeight ;
            alignas(64) std::array<T , OutputSize                 > OutputBias   ;
            alignas(64) std::array<OT, OutputSize                 > Output       ;
#elifdef __AVX2__
            alignas(32) std::array<T , InputSize * HiddenSize     > FeatureWeight;
            alignas(32) std::array<T , HiddenSize                 > FeatureBias  ;
            alignas(32) std::array<T , HiddenSize * 2 * OutputSize> OutputWeight ;
            alignas(32) std::array<T , OutputSize                 > OutputBias   ;
            alignas(32) std::array<OT, OutputSize                 > Output       ;
#else
            std::array<T , InputSize * HiddenSize     > FeatureWeight;
            std::array<T , HiddenSize                 > FeatureBias  ;
            std::array<T , HiddenSize * 2 * OutputSize> OutputWeight ;
            std::array<T , OutputSize                 > OutputBias   ;
            std::array<OT, OutputSize                 > Output       ;
#endif

            std::array<PerspectiveAccumulator<T, HiddenSize>, AccumulatorStackSize> Accumulators;
            uint16_t CurrentAccumulator = 0;

            /// \brief Initializes the accumulator stack.
            /// \details This function initializes the accumulator stack with empty accumulators.
            void InitializeAccumulatorStack()
            {
                PerspectiveAccumulator<T, HiddenSize> accumulator;
                std::fill(std::begin(Accumulators), std::end(Accumulators), accumulator);
            }

        public:
            /// \brief Constructs a new PerspectiveNetwork.
            /// \details This constructor initializes the network with undefined weights and biases.
            __attribute__((unused)) PerspectiveNetwork()
            {
                InitializeAccumulatorStack();
            }

            /// \brief Constructs a new PerspectiveNetwork.
            /// \param stream The binary file stream to read the network from.
            /// \details This constructor initializes the network with the weights and biases read from the stream.
            __attribute__((unused)) explicit PerspectiveNetwork(BinaryFileStream &stream)
            {
                InitializeAccumulatorStack();

                stream.ReadArray(FeatureWeight);
                stream.ReadArray(FeatureBias  );
                stream.ReadArray(OutputWeight );
                stream.ReadArray(OutputBias   );
            }

            /// \brief Constructs a new PerspectiveNetwork.
            /// \param stream The binary memory stream to read the network from.
            /// \details This constructor initializes the network with the weights and biases read from the stream.
            __attribute__((unused)) explicit PerspectiveNetwork(BinaryMemoryStream &stream)
            {
                InitializeAccumulatorStack();

                stream.ReadArray(FeatureWeight);
                stream.ReadArray(FeatureBias  );
                stream.ReadArray(OutputWeight );
                stream.ReadArray(OutputBias   );
            }

            /// \brief Constructs a new PerspectiveNetwork.
            /// \param stream The Marlinflow JSON stream to read the network from.
            /// \details This constructor initializes the network with the weights and biases read from the stream.
            ///          Internally, this constructor also quantizes the weights and biases. It also permutes the
            ///          weights to ensure better performance with respect to the cache. This constructor is only
            ///          there to ensure compatibility with the Marlinflow JSON network format.
            __attribute__((unused)) explicit PerspectiveNetwork(MarlinflowStream &stream)
            {
                InitializeAccumulatorStack();

                stream.Read2DArray("ft.weight" , FeatureWeight, HiddenSize    , QuantizationFeature,
                                   true );
                stream.Read2DArray("out.weight", OutputWeight , HiddenSize * 2, QuantizationOutput ,
                                   false);

                stream.ReadArray("ft.bias" , FeatureBias, QuantizationFeature                     );
                stream.ReadArray("out.bias", OutputBias , QuantizationFeature * QuantizationOutput);
            }

            /// \brief Writes the network to a binary file stream.
            /// \param stream The binary file stream to write the network to.
            /// \details This function writes the weights and biases of the network to the stream.
            __attribute__((unused)) void WriteTo(BinaryFileStream &stream)
            {
                stream.WriteMode();

                stream.WriteArray(FeatureWeight);
                stream.WriteArray(FeatureBias  );
                stream.WriteArray(OutputWeight );
                stream.WriteArray(OutputBias   );
            }

            /// \brief Reset the accumulator stack counter.
            /// \details This function resets the accumulator stack counter to zero.
            __attribute__((unused)) inline void ResetAccumulator()
            {
                CurrentAccumulator = 0;
            }

            /// \brief Pushes the current accumulator to the stack.
            /// \details This function pushes the current accumulator to the stack. This is useful when you want to
            ///          efficiently update the accumulator with a new piece move, but you want to keep the current
            ///          accumulator for later use (such as undoing).
            __attribute__((unused)) inline void PushAccumulator()
            {
                Accumulators[CurrentAccumulator].CopyTo(Accumulators[++CurrentAccumulator]);

                assert(CurrentAccumulator < AccumulatorStackSize);
            }

            /// \brief Pulls the current accumulator from the stack.
            /// \details This function pulls the current accumulator from the stack. This is useful when you want to
            ///          undo a move and restore the previous accumulator.
            __attribute__((unused)) inline void PullAccumulator()
            {
                assert(CurrentAccumulator > 0);

                CurrentAccumulator--;
            }

            /// \brief Refreshes the current accumulator.
            /// \details This function refreshes the current accumulator with the bias, effectively resetting it to
            ///          the initial state before any pieces were accumulated.
            __attribute__((unused)) inline void RefreshAccumulator()
            {
                PerspectiveAccumulator<T, HiddenSize>& accumulator = Accumulators[CurrentAccumulator];
                accumulator.Zero();
                accumulator.LoadBias(FeatureBias);
            }

            /// \brief Efficiently updates the current accumulator with a new piece move.
            /// \param piece The piece that is moved.
            /// \param color The color of the piece that is moved.
            /// \param from The square the piece is moved from.
            /// \param to The square the piece is moved to.
            /// \details This function efficiently updates the current accumulator with a new piece move. This is
            ///          done by subtracting the piece from the square it is moved from and adding it to the square
            ///          it is moved to. This is much more efficient than calling RefreshAccumulator() and then
            ///          accumulating all pieces again.
            __attribute__((unused)) inline void EfficientlyUpdateAccumulator(const uint8_t piece, const uint8_t color,
                                                                             const uint8_t from, const uint8_t to)
            {
                // Calculate the stride necessary to get to the correct piece:
                const uint16_t pieceStride = piece * PieceStride;

                // Calculate the indices for the square of the piece that is moved with respect to both perspectives:
                const uint32_t whiteIndexFrom =  color       * ColorStride + pieceStride +        from;
                const uint32_t blackIndexFrom = (color ^ 1)  * ColorStride + pieceStride + (from ^ 56);

                // Calculate the indices for the square the piece is moved to with respect to both perspectives:
                const uint32_t whiteIndexTo   =  color       * ColorStride + pieceStride +          to;
                const uint32_t blackIndexTo   = (color ^ 1)  * ColorStride + pieceStride + (  to ^ 56);

                // Fetch the current accumulator:
                PerspectiveAccumulator<T, HiddenSize>& accumulator = Accumulators[CurrentAccumulator];

                // Efficiently update the accumulator:
                SIMD::SubtractAndAddToAll(accumulator.White, accumulator.Black,
                                          FeatureWeight,
                                          whiteIndexFrom * HiddenSize,
                                          whiteIndexTo   * HiddenSize,
                                          blackIndexFrom * HiddenSize,
                                          blackIndexTo   * HiddenSize);
            }

            /// \brief Efficiently updates the current accumulator with a new piece insertion or removal.
            /// \tparam Operation The operation to perform on the accumulator.
            /// \param piece The piece that is inserted or removed.
            /// \param color The color of the piece that is inserted or removed.
            /// \param sq The square the piece is inserted or removed from.
            /// \details This function efficiently updates the current accumulator with a new piece insertion or
            ///          removal. This is done by adding or subtracting the piece from the square it is inserted
            ///          or removed from. This is much more efficient than calling the non-templated version of
            ///          this function.
            /// \see MantaRay::AccumulatorOperation for the available operations.
            template<AccumulatorOperation Operation>
            __attribute__((unused)) inline void EfficientlyUpdateAccumulator(const uint8_t piece, const uint8_t color,
                                                                             const uint8_t sq)
            {
                // Calculate the stride necessary to get to the correct piece:
                const uint16_t pieceStride = piece * PieceStride;

                // Calculate the indices for the square of the piece that is inserted or removed with respect to both
                // perspectives:
                const uint32_t whiteIndex =  color      * ColorStride + pieceStride +  sq      ;
                const uint32_t blackIndex = (color ^ 1) * ColorStride + pieceStride + (sq ^ 56);

                // Fetch the current accumulator:
                PerspectiveAccumulator<T, HiddenSize>& accumulator = Accumulators[CurrentAccumulator];

                // Efficiently update the accumulator:
                if (Operation == AccumulatorOperation::Activate)
                    SIMD::AddToAll(accumulator.White,
                                   accumulator.Black,
                                   FeatureWeight,
                                   whiteIndex * HiddenSize,
                                   blackIndex * HiddenSize);

                else SIMD::SubtractFromAll(accumulator.White,
                                           accumulator.Black,
                                           FeatureWeight,
                                           whiteIndex * HiddenSize,
                                           blackIndex * HiddenSize);
            }

            /// \brief Evaluates the network with respect to the current accumulator.
            /// \param colorToMove The color to move.
            /// \return The evaluation of the network with respect to the current accumulator.
            /// \details This function evaluates the network with respect to the current accumulator. The
            ///          accumulator is assumed to be up to date with the current position. The evaluation is
            ///          returned as the output type of the network.
            __attribute__((unused)) inline OT Evaluate(const uint8_t colorToMove)
            {
                // Fetch the current accumulator:
                PerspectiveAccumulator<T, HiddenSize>& accumulator = Accumulators[CurrentAccumulator];

                // Activate, flatten, and forward-propagate the accumulator to evaluate the network:
                if (colorToMove == 0) SIMD::ActivateFlattenAndForward<Activation>(
                        accumulator.White,
                        accumulator.Black,
                        OutputWeight,
                        OutputBias,
                        Output,
                        0);
                else                  SIMD::ActivateFlattenAndForward<Activation>(
                        accumulator.Black,
                        accumulator.White,
                        OutputWeight,
                        OutputBias,
                        Output,
                        0);

                // Scale the output with respect to the quantization and return it:
                return Output[0] * Scale / (QuantizationFeature * QuantizationOutput);
            }

    };

} // MantaRay

#endif //MANTARAY_PERSPECTIVENNUE_H
