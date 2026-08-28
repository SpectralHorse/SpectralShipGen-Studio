#include "RegressionSuites.h"

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "AttributeRerollStudio.h"
#include "GenerationDomainReroll.h"
#include "PreviewCommandPanel.h"
#include "ShipGenerationSeeds.h"
#include "ShipGenerationSettings.h"
#include "ShipGenerator.h"

namespace
{
    using namespace PixelShipGenerator;
    using namespace PixelShipGeneratorPreview;

    PreviewGenerationRecipe makeRecipe(ShipDimensions dimensions = { 96u, 64u })
    {
        PreviewGenerationRecipe recipe;
        recipe.Seeds = deriveShipGenerationSeeds(0x51A77B1E5EED1234ull);
        recipe.Dimensions = dimensions;
        recipe.Style = ShipStyle::HEAVY;
        recipe.Faction = ShipFactionType::FRONTIER;
        recipe.DetailDensity = 64u;
        recipe.AsymmetricDetailChance = 17u;
        recipe.AttachmentsEnabled = true;
        return recipe;
    }

    ShipGenerationSettings makeSettings(const PreviewGenerationRecipe& recipe)
    {
        ShipGenerationSettings settings;
        settings.Seed = recipe.Seeds.Master;
        settings.Dimensions = recipe.Dimensions;
        settings.Style = recipe.Style;
        settings.Faction = recipe.Faction;
        settings.DetailDensity = recipe.DetailDensity;
        settings.AsymmetricDetailChance = recipe.AsymmetricDetailChance;
        settings.AttachmentsEnabled = recipe.AttachmentsEnabled;
        settings.SeedOverrides.Structure = recipe.Seeds.Structure;
        settings.SeedOverrides.Palette = recipe.Seeds.Palette;
        settings.SeedOverrides.Details = recipe.Seeds.Details;
        settings.SeedOverrides.Attachments = recipe.Seeds.Attachments;
        settings.DomainSeedOverrides = recipe.DomainSeedOverrides;
        settings.RandomStreamMode = recipe.RandomStreamMode;
        return settings;
    }

    bool masksEqual(const PixelMask& first, const PixelMask& second)
    {
        if (first.getWidth() != second.getWidth() || first.getHeight() != second.getHeight()) { return false; }
        for (uint32_t y = 0u; y < first.getHeight(); ++y)
        {
            for (uint32_t x = 0u; x < first.getWidth(); ++x)
            {
                if (first.get(x, y) != second.get(x, y)) { return false; }
            }
        }
        return true;
    }

    bool checkBaseAndRepeatedCandidates()
    {
        const PreviewGenerationRecipe base = makeRecipe();
        AttributeRerollStudioState studio;
        beginAttributeRerollStudio(studio, base);
        if (!studio.Active || studio.CandidateValid || studio.BaseRecipe != base || studio.CandidateRecipe != base || hasSelectedAttributeRerollDomains(studio))
        {
            std::cerr << "Studio entry did not preserve a clean BaseRecipe.\n";
            return false;
        }

        setAttributeRerollDomainSelected(studio, GenerationDomain::COCKPIT, true);
        setAttributeRerollDomainSelected(studio, GenerationDomain::HULL_LAYERS, true);
        const uint64_t firstSeed = 0x1111222233334444ull;
        const PreviewGenerationRecipe firstCandidate = generateAttributeRerollCandidate(studio, firstSeed);
        if (!studio.CandidateValid || studio.CandidateSequence != 1u || studio.BaseRecipe != base)
        {
            std::cerr << "First candidate mutated BaseRecipe or failed to become valid.\n";
            return false;
        }

        const GenerationDomainSeeds baseSeeds = resolveGenerationDomainSeeds(base.Seeds, base.DomainSeedOverrides, base.RandomStreamMode);
        const GenerationDomainSeeds firstSeeds = resolveGenerationDomainSeeds(firstCandidate.Seeds, firstCandidate.DomainSeedOverrides, firstCandidate.RandomStreamMode);
        for (std::size_t index = 0u; index < GenerationDomainCount; ++index)
        {
            const GenerationDomain domain = static_cast<GenerationDomain>(index);
            const bool selected = domain == GenerationDomain::COCKPIT || domain == GenerationDomain::HULL_LAYERS;
            if ((firstSeeds.Values[index] != baseSeeds.Values[index]) != selected)
            {
                std::cerr << "Candidate changed the wrong domain seed at " << getGenerationDomainName(domain) << ".\n";
                return false;
            }
        }

        AttributeRerollStudioState duplicate;
        beginAttributeRerollStudio(duplicate, base);
        setAttributeRerollDomainSelected(duplicate, GenerationDomain::COCKPIT, true);
        setAttributeRerollDomainSelected(duplicate, GenerationDomain::HULL_LAYERS, true);
        if (generateAttributeRerollCandidate(duplicate, firstSeed) != firstCandidate)
        {
            std::cerr << "Same BaseRecipe/selection/reroll seed did not reproduce the candidate recipe.\n";
            return false;
        }

        const uint64_t secondSeed = 0x9999AAAABBBBCCCCull;
        const PreviewGenerationRecipe secondCandidate = generateAttributeRerollCandidate(studio, secondSeed);
        const PreviewGenerationRecipe expectedSecond = rerollGenerationDomains(base, { GenerationDomain::COCKPIT, GenerationDomain::HULL_LAYERS }, secondSeed);
        if (studio.CandidateSequence != 2u || studio.BaseRecipe != base || secondCandidate != expectedSecond)
        {
            std::cerr << "Repeated reroll accumulated from the previous candidate instead of BaseRecipe.\n";
            return false;
        }
        return true;
    }

    bool checkPresetsAndRectangularRecipe()
    {
        AttributeRerollStudioState studio;
        const PreviewGenerationRecipe base = makeRecipe({ 48u, 64u });
        beginAttributeRerollStudio(studio, base);
        selectAttributeRerollParentChannel(studio, GenerationSeedChannel::STRUCTURE);
        for (std::size_t index = 0u; index < GenerationDomainCount; ++index)
        {
            const GenerationDomain domain = static_cast<GenerationDomain>(index);
            const bool expected = getGenerationDomainParentChannel(domain) == GenerationSeedChannel::STRUCTURE;
            if (studio.SelectedDomains[index] != expected)
            {
                std::cerr << "Structural preset selected the wrong domain set.\n";
                return false;
            }
        }

        selectAttributeRerollAppearanceDomains(studio);
        for (std::size_t index = 0u; index < GenerationDomainCount; ++index)
        {
            const GenerationDomain domain = static_cast<GenerationDomain>(index);
            const bool expected = domain == GenerationDomain::PALETTE || domain == GenerationDomain::DETAILS;
            if (studio.SelectedDomains[index] != expected)
            {
                std::cerr << "Appearance preset selected the wrong domain set.\n";
                return false;
            }
        }

        selectAllAttributeRerollDomains(studio);
        if (getSelectedAttributeRerollDomains(studio).size() != GenerationDomainCount)
        {
            std::cerr << "Select All did not select every domain.\n";
            return false;
        }
        clearAttributeRerollDomains(studio);
        if (hasSelectedAttributeRerollDomains(studio))
        {
            std::cerr << "Clear did not clear every domain.\n";
            return false;
        }

        setAttributeRerollDomainSelected(studio, GenerationDomain::WEAPONS, true);
        const PreviewGenerationRecipe candidate = generateAttributeRerollCandidate(studio, 0xABCD1234567890EFull);
        if (candidate.Dimensions != base.Dimensions)
        {
            std::cerr << "Rectangular dimensions changed during domain reroll.\n";
            return false;
        }
        return true;
    }

    bool checkPaletteOnlyGeometryPreservation()
    {
        const PreviewGenerationRecipe base = makeRecipe({ 64u, 96u });
        AttributeRerollStudioState studio;
        beginAttributeRerollStudio(studio, base);
        setAttributeRerollDomainSelected(studio, GenerationDomain::PALETTE, true);
        const PreviewGenerationRecipe candidate = generateAttributeRerollCandidate(studio, 0xF00DFACE12345678ull);

        ShipGenerator generator;
        const GeneratedShip baseShip = generator.generate(makeSettings(base));
        const GeneratedShip candidateShip = generator.generate(makeSettings(candidate));
        if (!masksEqual(baseShip.HullMask, candidateShip.HullMask) || !masksEqual(baseShip.CockpitMask, candidateShip.CockpitMask) || !masksEqual(baseShip.EngineMask, candidateShip.EngineMask) || !masksEqual(baseShip.EngineExhaustMask, candidateShip.EngineExhaustMask) || !masksEqual(baseShip.AttachmentMask, candidateShip.AttachmentMask))
        {
            std::cerr << "Palette-only studio reroll changed structural geometry.\n";
            return false;
        }
        return true;
    }

    bool checkCommandPanelStudioLayout()
    {
        PreviewCommandPanel panel;
        const auto countCommand = [&](PreviewCommandType type)
            {
                std::size_t count = 0u;
                for (const PreviewCommandPanelButton& button : panel.getButtons()) { if (button.Command.Type == type) { ++count; } }
                return count;
            };
        if (countCommand(PreviewCommandType::OPEN_REROLL_STUDIO) != 1u || countCommand(PreviewCommandType::REROLL_STUDIO_TOGGLE_DOMAIN) != 0u)
        {
            std::cerr << "Normal command panel does not expose exactly one Reroll Studio entry point.\n";
            return false;
        }

        PreviewCommandPanelState state;
        state.Mode = PreviewCommandPanelMode::REROLL_STUDIO;
        state.Enabled.fill(true);
        panel.updateState(state);
        if (panel.getMode() != PreviewCommandPanelMode::REROLL_STUDIO)
        {
            std::cerr << "Command panel did not enter Reroll Studio layout.\n";
            return false;
        }

        const PreviewCommandPanelButton* cockpitButton = nullptr;
        for (const PreviewCommandPanelButton& button : panel.getButtons())
        {
            if (button.Command.Type == PreviewCommandType::REROLL_STUDIO_TOGGLE_DOMAIN && button.Command.Value == static_cast<uint32_t>(GenerationDomain::COCKPIT))
            {
                cockpitButton = &button;
                break;
            }
        }
        if (cockpitButton == nullptr)
        {
            std::cerr << "Cockpit checkbox is missing from studio layout.\n";
            return false;
        }

        const sf::Vector2f center(cockpitButton->Bounds.left + cockpitButton->Bounds.width * 0.5f, cockpitButton->Bounds.top + cockpitButton->Bounds.height * 0.5f);
        panel.onMousePress(center);
        const std::optional<PreviewCommand> command = panel.onMouseRelease(center);
        if (!command.has_value() || command->Type != PreviewCommandType::REROLL_STUDIO_TOGGLE_DOMAIN || command->Value != static_cast<uint32_t>(GenerationDomain::COCKPIT))
        {
            std::cerr << "Mouse interaction did not emit the same Cockpit toggle command represented by the UI.\n";
            return false;
        }

        state.RerollStudioSelectedDomains[static_cast<std::size_t>(GenerationDomain::COCKPIT)] = true;
        panel.updateState(state);
        for (const PreviewCommandPanelButton& button : panel.getButtons())
        {
            if (button.Command.Type == PreviewCommandType::REROLL_STUDIO_TOGGLE_DOMAIN && button.Command.Value == static_cast<uint32_t>(GenerationDomain::COCKPIT))
            {
                if (!button.Active || button.Label.find("[X]") != 0u)
                {
                    std::cerr << "Selected Cockpit domain is not visually represented as selected.\n";
                    return false;
                }
            }
        }
        return true;
    }
}

int PixelShipGeneratorTests::runAttributeRerollStudioRegression()
{
    bool success = true;
    success = checkBaseAndRepeatedCandidates() && success;
    success = checkPresetsAndRectangularRecipe() && success;
    success = checkPaletteOnlyGeometryPreservation() && success;
    success = checkCommandPanelStudioLayout() && success;
    return success ? 0 : 1;
}
