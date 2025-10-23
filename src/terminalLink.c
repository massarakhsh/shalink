#include "terminal.h"
#include "interop.h"

void terminalLinkInsert(Terminal *term, Link *link) {
    link->predLink = term->lastLink;
    if (term->lastLink != NULL) term->lastLink->nextLink = link;
    else term->firstLink = link;
    term->lastLink = link;
    link->nextLink = NULL;
}

void terminalLinkExtruct(Terminal *term, Link *link) {
    Link *pred = link->predLink;
    Link *next = link->nextLink;
    if (pred) {
        pred->nextLink = next;
        link->predLink = NULL;
    }
    else term->firstLink = next;
    if (next) {
        next->predLink = pred;
        link->nextLink = NULL;
    }
    else term->lastLink = pred;
}

void terminalLinkRoll(Terminal *term) {
    Link *first = term->firstLink;
    Link *last = term->lastLink;
    if (first == NULL || last == NULL || first == last) return;
    term->firstLink = first->nextLink;
    term->firstLink->predLink = NULL;
    last->nextLink = first;
    first->predLink = last;
    first->nextLink = NULL;
    term->lastLink = first;
}

