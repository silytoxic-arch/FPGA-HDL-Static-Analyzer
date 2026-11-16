VhdlNativeClockRelations vhdlNativeClockRelations12(
    const std::vector<parser::HdlToken>& tokens,
    std::size_t architecture, std::size_t limit,
    const parser::DesignUnitInfo& unit,
    const core::SourceFile& source,
    const std::set<std::string>& parsedClocks) {
    VhdlNativeClockRelations result;
    const auto staticActionNames = vhdlNativeStaticActionNames(
        tokens, architecture, limit, unit);
    // Native FUN_1804c5890 is a nonrecursive two-level collector: it accepts
    // only direct AST_ARCHITECTURE(0x75) children of the input root and direct
    // AST_ALWAYS/process(0x52) children of each architecture.  Do not let a
    // process below generate/block nodes widen the class-1/class-2 relation set.
    // ev_fdfe77d6d9ec134b166d321c11ef651b5b7fbc15bba2280f725f8afe72d05f91
    const auto directArchitectureToken = vhdlDirectArchitectureTokenMask(
        tokens, architecture, limit);

    for (auto process = architecture; process < limit; ++process) {
        if (lower(tokens[process].text) != "process" ||
            !directArchitectureToken[process] ||
            (process > architecture && lower(tokens[process - 1].text) == "end"))
            continue;
        auto end = process + 1;
        while (end + 1 < limit &&
               !(lower(tokens[end].text) == "end" &&
                 lower(tokens[end + 1].text) == "process")) ++end;
        if (end + 1 >= limit) continue;
        auto body = process + 1;
        int parentheses = 0;
        for (; body < end; ++body) {
            if (tokens[body].text == "(") ++parentheses;
            else if (tokens[body].text == ")" && parentheses > 0) --parentheses;
            if (parentheses == 0 && lower(tokens[body].text) == "begin") {
                ++body;
                break;
            }
        }
        if (body >= end) {
            process = end + 1;
            continue;
        }

        struct SourceIf { std::size_t begin{}, end{}; };
        std::vector<SourceIf> sourceIfs;
        std::optional<std::size_t> topLevelIf;
        unsigned ifDepth = 0;
        unsigned caseDepth = 0;
        unsigned loopDepth = 0;
        for (auto token = body; token + 1 < end; ++token) {
            const auto word = lower(tokens[token].text);
            if (word == "end") {
                const auto closing = lower(tokens[token + 1].text);
                if (closing == "if" && ifDepth != 0) {
                    --ifDepth;
                    if (ifDepth == 0 && topLevelIf) {
                        sourceIfs.push_back({*topLevelIf, token});
                        topLevelIf.reset();
                    }
                } else if (closing == "case" && caseDepth != 0) {
                    --caseDepth;
                } else if (closing == "loop" && loopDepth != 0) {
                    --loopDepth;
                }
                if (closing == "if" || closing == "case" || closing == "loop")
                    ++token;
                continue;
            }
            if (word == "if") {
                if (ifDepth == 0 && caseDepth == 0 && loopDepth == 0)
                    topLevelIf = token;
                ++ifDepth;
            } else if (word == "case") {
                ++caseDepth;
            } else if (word == "loop") {
                ++loopDepth;
            }
        }

        for (const auto& sourceIf : sourceIfs) {
            auto firstThen = sourceIf.begin + 1;
            int conditionDepth = 0;
            for (; firstThen < sourceIf.end; ++firstThen) {
                if (tokens[firstThen].text == "(") ++conditionDepth;
                else if (tokens[firstThen].text == ")" && conditionDepth > 0)
                    --conditionDepth;
                if (conditionDepth == 0 && lower(tokens[firstThen].text) == "then")
                    break;
            }
            if (firstThen >= sourceIf.end) continue;

            const auto mainEdge = findFirstVhdlEdgeCondition(
                source, tokens, sourceIf.begin + 1, firstThen);
            if (mainEdge.status == VhdlEdgeConditionStatus::Matched) {
                if (parsedClocks.contains(mainEdge.clockName)) {
                    result.names.insert(mainEdge.clockName); // relation key 2
                    result.sourceLines.insert(tokens[sourceIf.begin].line);
                    if (firstThen + 1 < sourceIf.end &&
                        lower(tokens[firstThen + 1].text) == "if")
                        result.sourceLines.insert(tokens[firstThen + 1].line);
                }
                continue;
            }

            std::vector<std::size_t> elsifTokens;
            std::size_t elseToken = sourceIf.end;
            unsigned nestedIf = 0;
            for (auto token = firstThen + 1; token < sourceIf.end; ++token) {
                const auto word = lower(tokens[token].text);
                if (word == "end" && token + 1 < sourceIf.end &&
                    lower(tokens[token + 1].text) == "if") {
                    if (nestedIf != 0) --nestedIf;
                    ++token;
                    continue;
                }
                if (word == "if") {
                    ++nestedIf;
                } else if (nestedIf == 0 && word == "elsif") {
                    elsifTokens.push_back(token);
                } else if (nestedIf == 0 && word == "else") {
                    elseToken = token;
                    break;
                }
            }
            const auto firstBranchEnd = !elsifTokens.empty()
                ? elsifTokens.front()
                : elseToken < sourceIf.end ? elseToken : sourceIf.end;
            bool staticAction = hasVhdlNativeStaticAssignment(
                tokens, firstThen + 1, firstBranchEnd, staticActionNames);
            std::optional<VhdlEdgeCondition> relationEdge;
            std::size_t extraEdges = 0;
            for (std::size_t branch = 0; branch < elsifTokens.size(); ++branch) {
                const auto elsifToken = elsifTokens[branch];
                const auto branchEnd = branch + 1 < elsifTokens.size()
                    ? elsifTokens[branch + 1]
                    : elseToken < sourceIf.end ? elseToken : sourceIf.end;
                auto thenToken = elsifToken + 1;
                conditionDepth = 0;
                for (; thenToken < branchEnd; ++thenToken) {
                    if (tokens[thenToken].text == "(") ++conditionDepth;
                    else if (tokens[thenToken].text == ")" && conditionDepth > 0)
                        --conditionDepth;
                    if (conditionDepth == 0 && lower(tokens[thenToken].text) == "then")
                        break;
                }
                if (thenToken >= branchEnd) continue;
                const auto edge = findFirstVhdlEdgeCondition(
                    source, tokens, elsifToken + 1, thenToken);
                if (edge.status == VhdlEdgeConditionStatus::Matched) {
                    if (relationEdge) ++extraEdges;
                    else relationEdge = edge;
                } else if (extraEdges == 0) {
                    staticAction = staticAction || hasVhdlNativeStaticAssignment(
                        tokens, thenToken + 1, branchEnd, staticActionNames);
                }
            }

            std::optional<SourceIf> linkedElseIf;
            if (elseToken < sourceIf.end) {
                std::optional<std::size_t> directIf;
                unsigned elseIfDepth = 0;
                unsigned elseCaseDepth = 0;
                unsigned elseLoopDepth = 0;
                for (auto token = elseToken + 1; token + 1 < sourceIf.end; ++token) {
                    const auto word = lower(tokens[token].text);
                    if (word == "end") {
                        const auto closing = lower(tokens[token + 1].text);
                        if (closing == "if" && elseIfDepth != 0) {
                            --elseIfDepth;
                            if (elseIfDepth == 0 && directIf) {
                                linkedElseIf = SourceIf{*directIf, token};
                                directIf.reset();
                            }
                        } else if (closing == "case" && elseCaseDepth != 0) {
                            --elseCaseDepth;
                        } else if (closing == "loop" && elseLoopDepth != 0) {
                            --elseLoopDepth;
                        }
                        if (closing == "if" || closing == "case" ||
                            closing == "loop") ++token;
                        continue;
                    }
                    if (word == "if") {
                        if (elseIfDepth == 0 && elseCaseDepth == 0 &&
                            elseLoopDepth == 0) directIf = token;
                        ++elseIfDepth;
                    } else if (word == "case") {
                        ++elseCaseDepth;
                    } else if (word == "loop") {
                        ++elseLoopDepth;
                    }
                }
            }
            if (linkedElseIf) {
                auto linkedThen = linkedElseIf->begin + 1;
                conditionDepth = 0;
                for (; linkedThen < linkedElseIf->end; ++linkedThen) {
                    if (tokens[linkedThen].text == "(") ++conditionDepth;
                    else if (tokens[linkedThen].text == ")" && conditionDepth > 0)
                        --conditionDepth;
                    if (conditionDepth == 0 && lower(tokens[linkedThen].text) == "then")
                        break;
                }
                if (linkedThen < linkedElseIf->end) {
                    const auto edge = findFirstVhdlEdgeCondition(
                        source, tokens, linkedElseIf->begin + 1, linkedThen);
                    if (edge.status == VhdlEdgeConditionStatus::Matched) {
                        if (relationEdge) ++extraEdges;
                        else relationEdge = edge;
                    } else if (extraEdges == 0) {
                        staticAction = staticAction || hasVhdlNativeStaticAssignment(
                            tokens, linkedElseIf->begin, linkedElseIf->end,
                            staticActionNames);
                    }
                }
            }

            if (!staticAction || !relationEdge ||
                !parsedClocks.contains(relationEdge->clockName)) continue;
            result.names.insert(relationEdge->clockName); // relation key 1
            result.sourceLines.insert(tokens[sourceIf.begin].line);
            for (const auto elsifToken : elsifTokens)
                result.sourceLines.insert(tokens[elsifToken].line);
            if (linkedElseIf)
                result.sourceLines.insert(tokens[linkedElseIf->begin].line);
        }
        process = end + 1;
    }
    return result;
}
