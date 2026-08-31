#include "AttributeRerollStudio.h"

#include <cstddef>

#include <SpectralShipGen/GenerationDomainReroll.h>

namespace SpectralShipGenStudioPreview
{
    void beginAttributeRerollStudio(AttributeRerollStudioState& state, const PreviewGenerationRecipe& baseRecipe)
    {
        state = {};
        state.BaseRecipe = baseRecipe;
        state.CandidateRecipe = baseRecipe;
        state.Active = true;
    }

    void resetAttributeRerollStudio(AttributeRerollStudioState& state)
    {
        state = {};
    }

    void setAttributeRerollDomainSelected(AttributeRerollStudioState& state, SpectralShipGen::GenerationDomain domain, bool selected)
    {
        const std::size_t index = static_cast<std::size_t>(domain);
        if (index < state.SelectedDomains.size()) { state.SelectedDomains[index] = selected; }
    }

    void toggleAttributeRerollDomain(AttributeRerollStudioState& state, SpectralShipGen::GenerationDomain domain)
    {
        const std::size_t index = static_cast<std::size_t>(domain);
        if (index < state.SelectedDomains.size()) { state.SelectedDomains[index] = !state.SelectedDomains[index]; }
    }

    void selectAllAttributeRerollDomains(AttributeRerollStudioState& state)
    {
        state.SelectedDomains.fill(true);
    }

    void clearAttributeRerollDomains(AttributeRerollStudioState& state)
    {
        state.SelectedDomains.fill(false);
    }

    void selectAttributeRerollParentChannel(AttributeRerollStudioState& state, SpectralShipGen::GenerationSeedChannel channel, bool clearExisting)
    {
        if (clearExisting) { clearAttributeRerollDomains(state); }
        for (std::size_t index = 0u; index < state.SelectedDomains.size(); ++index)
        {
            const SpectralShipGen::GenerationDomain domain = static_cast<SpectralShipGen::GenerationDomain>(index);
            if (SpectralShipGen::getGenerationDomainParentChannel(domain) == channel) { state.SelectedDomains[index] = true; }
        }
    }

    void selectAttributeRerollAppearanceDomains(AttributeRerollStudioState& state, bool clearExisting)
    {
        if (clearExisting) { clearAttributeRerollDomains(state); }
        setAttributeRerollDomainSelected(state, SpectralShipGen::GenerationDomain::PALETTE, true);
        setAttributeRerollDomainSelected(state, SpectralShipGen::GenerationDomain::DETAILS, true);
    }

    bool hasSelectedAttributeRerollDomains(const AttributeRerollStudioState& state)
    {
        for (const bool selected : state.SelectedDomains) { if (selected) { return true; } }
        return false;
    }

    std::vector<SpectralShipGen::GenerationDomain> getSelectedAttributeRerollDomains(const AttributeRerollStudioState& state)
    {
        std::vector<SpectralShipGen::GenerationDomain> result;
        result.reserve(state.SelectedDomains.size());
        for (std::size_t index = 0u; index < state.SelectedDomains.size(); ++index)
        {
            if (state.SelectedDomains[index]) { result.push_back(static_cast<SpectralShipGen::GenerationDomain>(index)); }
        }
        return result;
    }

    PreviewGenerationRecipe generateAttributeRerollCandidate(AttributeRerollStudioState& state, uint64_t rerollSeed)
    {
        if (!state.Active || !hasSelectedAttributeRerollDomains(state)) { return state.BaseRecipe; }

        state.CandidateRerollSeed = rerollSeed;
        state.CandidateRecipe = SpectralShipGen::rerollGenerationDomains(state.BaseRecipe, getSelectedAttributeRerollDomains(state), rerollSeed);
        state.CandidateValid = true;
        ++state.CandidateSequence;
        return state.CandidateRecipe;
    }
}
