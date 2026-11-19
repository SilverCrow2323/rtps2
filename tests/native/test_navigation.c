#include <assert.h>
#include <stdio.h>

#include "nexus_navigation.h"

int main(void)
{
    NexusNavState state;

    nexus_nav_init(&state);
    assert(state.page == NEXUS_PAGE_HOME);
    assert(state.selected == 0);
    assert(state.in_section == 0);

    nexus_nav_apply(&state, NEXUS_KEY_UP);
    assert(state.selected == 5);

    nexus_nav_apply(&state, NEXUS_KEY_DOWN);
    assert(state.selected == 0);

    nexus_nav_apply(&state, NEXUS_KEY_CONFIRM);
    assert(state.page == NEXUS_PAGE_GAMES);
    assert(state.in_section == 1);

    nexus_nav_apply(&state, NEXUS_KEY_NEXT);
    assert(state.page == NEXUS_PAGE_PS1);

    nexus_nav_apply(&state, NEXUS_KEY_PREVIOUS);
    assert(state.page == NEXUS_PAGE_GAMES);

    nexus_nav_apply(&state, NEXUS_KEY_BACK);
    assert(state.page == NEXUS_PAGE_HOME);
    assert(state.in_section == 0);

    nexus_nav_apply(&state, NEXUS_KEY_NEXT);
    assert(state.selected == 1);
    nexus_nav_apply(&state, NEXUS_KEY_CONFIRM);
    assert(state.page == NEXUS_PAGE_PS1);
    nexus_nav_apply(&state, NEXUS_KEY_HOME);
    assert(state.page == NEXUS_PAGE_HOME);
    assert(state.selected == 0);
    assert(state.in_section == 0);

    puts("native navigation tests: PASS");
    return 0;
}
