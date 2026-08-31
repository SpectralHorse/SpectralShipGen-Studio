#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include <SpectralShipGen/GenerationDomain.h>
#include "PreviewGenerationRecipe.h"

namespace SpectralShipGenStudioPreview
{
    struct AttributeRerollStudioState
    {
        PreviewGenerationRecipe BaseRecipe;
        PreviewGenerationRecipe CandidateRecipe;
        std::array<bool, SpectralShipGen::GenerationDomainCount> SelectedDomains = {};
        uint64_t CandidateRerollSeed = 0u;
        uint32_t CandidateSequence = 0u;
        bool Active = false;
        bool CandidateValid = false;
    };

    void beginAttributeRerollStudio(AttributeRerollStudioState& state, const PreviewGenerationRecipe& baseRecipe);
    void resetAttributeRerollStudio(AttributeRerollStudioState& state);
    void setAttributeRerollDomainSelected(AttributeRerollStudioState& state, SpectralShipGen::GenerationDomain domain, bool selected);
    void toggleAttributeRerollDomain(AttributeRerollStudioState& state, SpectralShipGen::GenerationDomain domain);
    void selectAllAttributeRerollDomains(AttributeRerollStudioState& state);
    void clearAttributeRerollDomains(AttributeRerollStudioState& state);
    void selectAttributeRerollParentChannel(AttributeRerollStudioState& state, SpectralShipGen::GenerationSeedChannel channel, bool clearExisting = true);
    void selectAttributeRerollAppearanceDomains(AttributeRerollStudioState& state, bool clearExisting = true);
    bool hasSelectedAttributeRerollDomains(const AttributeRerollStudioState& state);
    std::vector<SpectralShipGen::GenerationDomain> getSelectedAttributeRerollDomains(const AttributeRerollStudioState& state);
    PreviewGenerationRecipe generateAttributeRerollCandidate(AttributeRerollStudioState& state, uint64_t rerollSeed);
}
