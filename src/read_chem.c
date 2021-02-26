#include "biort.h"

void ReadChem(const char dir[], ctrl_struct *ctrl, rttbl_struct *rttbl, chemtbl_struct chemtbl[],
    kintbl_struct kintbl[])
{
    int             i;
    char            cmdstr[MAXSTRING];
    char            temp_str[MAXSTRING];
    char            chemn[MAXSPS][MAXSTRING];
    char            fn[MAXSTRING];
    int             p_type[MAXSPS];
    int             lno = 0;
    FILE           *fp;

    // READ CHEM.TXT FILE
    sprintf(fn, "input/%s/chem.txt", dir);
    fp = fopen(fn, "r");

    NextLine(fp, cmdstr, &lno);
    ReadParam(cmdstr, "RECYCLE", 'i', fn, lno, &ctrl->recycle);
    printf("  Forcing recycle %d time(s). \n", ctrl->recycle);

    NextLine(fp, cmdstr, &lno);
    ReadParam(cmdstr, "ACTIVITY", 'i', fn, lno, &ctrl->actv_mode);
    printf("  Activity correction is set to %d. \n", ctrl->actv_mode);

    NextLine(fp, cmdstr, &lno);
    ReadParam(cmdstr, "RELMIN", 'i', fn, lno, &ctrl->rel_min);
    switch (ctrl->rel_min)
    {
        case 0:
            printf("  Using absolute mineral volume fraction. \n");
            break;
        case 1:
            printf("  Using relative mineral volume fraction. \n");
            break;
        default:
            break;
    }

    NextLine(fp, cmdstr, &lno);
    ReadParam(cmdstr, "TRANSPORT_ONLY", 'i', fn, lno, &ctrl->transpt);
    switch (ctrl->transpt)
    {
        case KIN_REACTION:
            printf("  Transport only mode disabled.\n");
            break;
        case TRANSPORT_ONLY:
            printf("  Transport only mode enabled. \n");
            break;
            // under construction.
        default:
            break;
    }

    NextLine(fp, cmdstr, &lno);
    ReadParam(cmdstr, "CEMENTATION", 'd', fn, lno, &rttbl->cementation);
    printf("  Cementation factor = %2.1f \n", rttbl->cementation);

    NextLine(fp, cmdstr, &lno);
    ReadParam(cmdstr, "TEMPERATURE", 'd', fn, lno, &rttbl->tmp);
    printf("  Temperature = %3.1f \n", rttbl->tmp);

    NextLine(fp, cmdstr, &lno);
    ReadParam(cmdstr, "SW_THRESHOLD", 'd', fn, lno, &rttbl->sw_thld);
    printf("  SW threshold = %.2f\n", rttbl->sw_thld);

    NextLine(fp, cmdstr, &lno);
    ReadParam(cmdstr, "SW_EXP", 'd', fn, lno, &rttbl->sw_exp);
    printf("  SW exponent = %.2f\n", rttbl->sw_exp);

    // Count numbers of species and reactions
    FindLine(fp, "PRIMARY_SPECIES", &lno, fn);
    rttbl->num_stc = CountLines(fp, cmdstr, 1, "SECONDARY_SPECIES");
    rttbl->num_ssc = CountLines(fp, cmdstr, 1, "MINERAL_KINETICS");
    rttbl->num_mkr = CountLines(fp, cmdstr, 1, "PRECIPITATION_CONC");
    rttbl->num_akr = 0;     // Not implemented yet

    // Primary species block
    printf("\n Primary species block\n");
    printf("  %d chemical species specified. \n", rttbl->num_stc);
    FindLine(fp, "BOF", &lno, fn);
    FindLine(fp, "PRIMARY_SPECIES", &lno, fn);

    rttbl->num_spc = 0;
    rttbl->num_ads = 0;
    rttbl->num_cex = 0;
    rttbl->num_min = 0;

    for (i = 0; i < rttbl->num_stc; i++)
    {
        NextLine(fp, cmdstr, &lno);
        if (sscanf(cmdstr, "%s", chemn[i]) != 1)
        {
            printf("Error reading primary_species in %s near Line %d.\n", fn, lno);
        }
        p_type[i] = SpeciesType(dir, chemn[i]);

        switch (p_type[i])
        {
            case 0:     // Species type is 0 when it is not found in the database.
                printf("Error finding primary species %s in the database.\n", chemn[i]);
                exit(EXIT_FAILURE);
            case AQUEOUS:
                rttbl->num_spc++;
                break;
            case ADSORPTION:
                rttbl->num_ads++;
                break;
            case CATION_ECHG:
                rttbl->num_cex++;
                break;
            case MINERAL:
                rttbl->num_min++;
                break;
            case SECONDARY:
                printf("%s is a secondary species, but is listed as a primary species.\n"
                    "Error at Line %d in %s.\n", chemn[i], lno, fn);
                exit(EXIT_FAILURE);
                break;
            default:
                break;
        }
    }

    printf("  %d aqueous species specified. \n", rttbl->num_spc);
    printf("  %d surface complexation specified. \n", rttbl->num_ads);
    printf("  %d cation exchange specified. \n", rttbl->num_cex);
    printf("  %d minerals specified. \n", rttbl->num_min);

    SortChem(chemn, p_type, rttbl->num_stc, chemtbl);

    // Number of species that others depend on
    rttbl->num_sdc = rttbl->num_stc - rttbl->num_min;

    // Secondary_species block
    printf("\n Secondary species block\n");
    printf("  %d secondary species specified. \n", rttbl->num_ssc);
    FindLine(fp, "SECONDARY_SPECIES", &lno, fn);
    for (i = 0; i < rttbl->num_ssc; i++)
    {
        NextLine(fp, cmdstr, &lno);
        if (sscanf(cmdstr, "%s", chemtbl[rttbl->num_stc + i].name) != 1)
        {
            printf("Error reading secondary_species in %s near Line %d.\n", fn, lno);
        }

        if (SpeciesType(dir, chemtbl[rttbl->num_stc + i].name) == 0)
        {
            printf("Error finding secondary species %s in the database.\n",
                chemtbl[rttbl->num_stc + i].name);
            exit(EXIT_FAILURE);
        }
    }

    // Minerals block
    printf("\n Minerals block\n");
    printf("  %d mineral kinetic reaction(s) specified. \n", rttbl->num_mkr);
    FindLine(fp, "MINERAL_KINETICS", &lno, fn);

    for (i = 0; i < rttbl->num_mkr; i++)
    {
        NextLine(fp, cmdstr, &lno);
        if (sscanf(cmdstr, "%s %*s %s", temp_str, kintbl[i].label) != 2)
        {
            printf("Error reading mineral information in %s near Line %d.\n", fn, lno);
            exit(EXIT_FAILURE);
        }

        printf("  Kinetic reaction on '%s' is specified, label '%s'.\n", temp_str, kintbl[i].label);

        kintbl[i].position = FindChem(temp_str, rttbl->num_stc, chemtbl);

        if (kintbl[i].position < 0)
        {
            printf("Error finding mineral %s in species table.\n", temp_str);
            exit(EXIT_FAILURE);
        }
        else
        {
            printf("  Position_check (num_mkr[i] vs num_stc[j]) (%d, %d)\n", i, kintbl[i].position);
        }
    }

    fclose(fp);
}

// This subroutine is used to find out what the input species is.
//   0) not found within database
//   1) aqueous
//   2) adsorption
//   3) cation exchange
//   4) mineral
int SpeciesType(const char dir[], const char chemn[])
{
    char            fn[MAXSTRING];
    char            tempn[MAXSTRING];
    char            cmdstr[MAXSTRING];
    int             lno = 0;
    FILE           *fp;

    sprintf(fn, "input/%s/cdbs.txt", dir);
    fp = fopen(fn, "r");

    if (strcmp(chemn, "pH") == 0)
    {
        return AQUEOUS;
    }

    sprintf(tempn, "'%s'", chemn);

    FindLine(fp, "BOF", &lno, "cdbs.txt");

    NextLine(fp, cmdstr, &lno);
    while (MatchWrappedKey(cmdstr, "'End of primary'") != 0)
    {
        if (MatchWrappedKey(cmdstr, tempn) == 0)
        {
            return AQUEOUS;
        }
        NextLine(fp, cmdstr, &lno);
    }

    while (MatchWrappedKey(cmdstr, "'End of secondary'") != 0)
    {
        if (MatchWrappedKey(cmdstr, tempn) == 0)
        {
            return 5;
        }
        NextLine(fp, cmdstr, &lno);
    }

    while (MatchWrappedKey(cmdstr, "'End of minerals'") != 0)
    {
        if (MatchWrappedKey(cmdstr, tempn) == 0)
        {
            return MINERAL;
        }
        NextLine(fp, cmdstr, &lno);
    }

    while (strcmp(cmdstr, "End of surface complexation\r\n") != 0 &&
        strcmp(cmdstr, "End of surface complexation\n") != 0)
    {
        // Notice that in CrunchFlow database, starting from surface complexation, there is not apostrophe marks around
        // blocking keywords
        if (MatchWrappedKey(cmdstr, tempn) == 0)
        {
            return ADSORPTION;
        }
        NextLine(fp, cmdstr, &lno);
    }

    while (!feof(fp))
    {
        if (MatchWrappedKey(cmdstr, tempn) == 0)
        {
            return CATION_ECHG;
        }
        NextLine(fp, cmdstr, &lno);
    }

    fclose(fp);

    return 0;
}

int MatchWrappedKey(const char cmdstr[], const char key[])
{
    char            optstr[MAXSTRING];

    if (sscanf(cmdstr, "'%[^']'", optstr) != 1)
    {
        return 1;
    }
    else
    {
        Wrap(optstr);
        return (strcmp(optstr, key) == 0) ? 0 : 1;
    }
}

void SortChem(char chemn[MAXSPS][MAXSTRING], const int p_type[MAXSPS], int nsps, chemtbl_struct chemtbl[])
{
    int             i, j;
    int             temp;
    int             rank[MAXSPS];
    int             ranked_type[MAXSPS];

    for (i = 0; i < nsps; i++)
    {
        rank[i] = i;
        ranked_type[i] = p_type[i];
    }

    for (i = 0; i < nsps - 1; i++)
    {
        for (j = 0; j < nsps - i - 1; j++)
        {
            if (ranked_type[j] > ranked_type[j + 1])
            {
                temp = rank[j];
                rank[j] = rank[j + 1];
                rank[j + 1] = temp;

                temp = ranked_type[j];
                ranked_type[j] = ranked_type[j + 1];
                ranked_type[j + 1] = temp;
            }
        }
    }

    for (i = 0; i < nsps; i++)
    {
        strcpy(chemtbl[i].name, chemn[rank[i]]);
        chemtbl[i].itype = p_type[rank[i]];
    }
}

int FindChem(const char chemn[MAXSTRING], int nsps, const chemtbl_struct  chemtbl[])
{
    int             i;
    int             ind = BADVAL;

    for (i = 0; i < nsps; i++)
    {
        if (strcmp(chemn, chemtbl[i].name) == 0)
        {
            ind = i;
            break;
        }
    }

    return ind;
}
