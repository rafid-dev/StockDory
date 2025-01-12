//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_UCISEARCH_H
#define STOCKDORY_UCISEARCH_H

#include "../../Backend/ThreadPool.h"

#include "../../Engine/Search.h"

namespace StockDory
{

    class UCISearchLogger
    {

        public:
            static void LogDepthIteration(const uint8_t depth, const uint8_t selectiveDepth, const int32_t evaluation,
                                          const uint64_t nodes, const uint64_t ttNodes,
                                          const StockDory::TimeControl::Milliseconds time, const std::string& pv)
            {
                std::stringstream output;

                int64_t displayedTime = time.count();
                displayedTime = std::max(displayedTime, static_cast<int64_t>(1));

                const auto nps =        static_cast<uint64_t>(static_cast<double>(nodes) /
                        (static_cast<double>(displayedTime) / static_cast<double>(1000)));

                output << "info ";
                output << "depth " << static_cast<uint16_t>(depth) << " ";
                output << "seldepth " << static_cast<uint16_t>(selectiveDepth) << " ";
                output << "score ";

                if (abs(evaluation) > Infinity - MaxDepth)
                    output << "mate " << (evaluation > 0 ? Infinity - evaluation : -Infinity - evaluation) / 2 << " ";
                else
                    output << "cp " << evaluation << " ";

                output << "nodes " << nodes << " ";
                output << "ttnodes " << ttNodes << " ";
                output << "time " << displayedTime << " ";
                output << "nps " << nps << " ";
                output << "pv " << pv;

                std::cout << output.str() << std::endl;
            }

            static void LogBestMove(const Move& move)
            {
                std::stringstream output;
                output << "bestmove " << move.ToString();

                std::cout << output.str() << std::endl;
            }

    };

#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnusedValue"
    class UCISearch
    {

        using Search = Search<UCISearchLogger>;

        private:
            Search EngineSearch;

            bool Running = false;

        public:
            UCISearch() = default;

            explicit UCISearch(const Board& board, const StockDory::TimeControl& timeControl,
                               const RepetitionHistory& repetitionHistory, const uint8_t halfMoveCounter)
            {
                EngineSearch = Search(board, timeControl, repetitionHistory, halfMoveCounter);
            }

            void Start(const uint8_t depth)
            {
                ThreadPool.enqueue_detach(
                    [this](const uint8_t depth) {
                        Running = true;
                        EngineSearch.IterativeDeepening(depth);
                        Running = false;
                    },
                    depth
                );
            }

            void Stop()
            {
                EngineSearch.ForceStop();
            }

            [[nodiscard]]
            bool IsRunning() const
            {
                return Running;
            }

    };
#pragma clang diagnostic pop

} // StockDory

#endif //STOCKDORY_UCISEARCH_H
