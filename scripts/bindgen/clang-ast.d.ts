export interface inner {
    id?: string;
    kind?: string;
    loc?: loc;
    range?: range;
    inner?: inner[];
    isImplicit?: boolean;
    name?: string;
    tagUsed?: string;
    implicit?: boolean;
    type?: type;
    decl?: decl;
    language?: string;
    hasBraces?: boolean;
    isReferenced?: boolean;
    previousDecl?: string;
    mangledName?: string;
    variadic?: boolean;
    inherited?: boolean;
    storageClass?: string;
    depth?: number;
    index?: number;
    completeDefinition?: boolean;
    definitionData?: definitionData;
    fixedUnderlyingType?: fixedUnderlyingType;
    valueCategory?: string;
    value?: boolean | string | number;
    isDependent?: boolean;
    isInstantiationDependent?: boolean;
    isPostfix?: boolean;
    opcode?: string;
    canOverflow?: boolean;
    isUsed?: boolean;
    ownedTagDecl?: ownedTagDecl;
    parentDeclContextId?: string;
    scopedEnumTag?: string;
    originalNamespace?: originalNamespace;
    constexpr?: boolean;
    explicitlyDefaulted?: string;
    inline?: boolean;
    visibility?: string;
    castKind?: string;
    referencedDecl?: referencedDecl;
    cc?: string;
    qualifiers?: string;
    access?: string;
    explicitlyDeleted?: boolean;
    message?: string;
    init?: string;
    isPartOfExplicitCast?: boolean;
    target?: target;
    argType?: argType;
    isArrow?: boolean;
    referencedMemberDecl?: string;
    templateName?: string;
    isExpr?: boolean;
    isAlias?: boolean;
    defaultArg?: defaultArg;
    bases?: bases[];
    isParameterPack?: boolean;
    isPack?: boolean;
    containsUnexpandedPack?: boolean;
    usesADL?: boolean;
    lookups?: lookups[] | any[];
    nonOdrUseReason?: string;
    foundReferencedDecl?: foundReferencedDecl;
    numElements?: number;
    hasElse?: boolean;
    computeLHSType?: computeLHSType;
    computeResultType?: computeResultType;
    storageDuration?: string;
    size?: number;
    qualifier?: string;
    isFunction?: boolean;
    const?: boolean;
    volatile?: boolean;
    refQualifier?: string;
    exceptionSpec?: string;
    isData?: boolean;
    transformKind?: string;
    member?: string;
    isConstexpr?: boolean;
    hasInClassInitializer?: boolean;
    list?: boolean;
    boundToLValueRef?: boolean;
    anyInit?: anyInit;
    ctorType?: ctorType;
    zeroing?: boolean;
    hadMultipleCandidates?: boolean;
    constructionKind?: string;
    virtual?: boolean;
    baseInit?: baseInit;
    path?: path[];
    nrvo?: boolean;
    conversionFunc?: conversionFunc;
    cleanupsHaveSideEffects?: boolean;
    temp?: string;
    dtor?: dtor;
    isGlobal?: boolean;
    isPlacement?: boolean;
    isInline?: boolean;
    immediate?: boolean;
    adl?: boolean;
    ['inherited from']?: InheritedFrom;
    hasInit?: boolean;
    initStyle?: string;
    operatorNewDecl?: operatorNewDecl;
    operatorDeleteDecl?: operatorDeleteDecl;
    pack_index?: number;
    mutable?: boolean;
    pure?: boolean;
    array_filler?: array_filler[];
    isInvalid?: boolean;
}

export interface loc {
    offset?: number;
    file?: string;
    line?: number;
    col?: number;
    tokLen?: number;
    includedFrom?: includedFrom;
    spellingLoc?: spellingLoc;
    expansionLoc?: expansionLoc;
}

export interface range {
    begin: begin;
    end: end;
}

export interface begin {
    offset?: number;
    col?: number;
    tokLen?: number;
    includedFrom?: includedFrom;
    line?: number;
    spellingLoc?: spellingLoc;
    expansionLoc?: expansionLoc;
    file?: string;
}

export interface end {
    offset?: number;
    line?: number;
    col?: number;
    tokLen?: number;
    includedFrom?: includedFrom;
    spellingLoc?: spellingLoc;
    expansionLoc?: expansionLoc;
}

export interface type {
    qualType: string;
    desugaredQualType?: string;
    typeAliasDeclId?: string;
}

export interface decl {
    id: string;
    kind?: string;
    name?: string;
}

export interface includedFrom {
    file: string;
}

export interface definitionData {
    canConstDefaultInit?: boolean;
    copyAssign: copyAssign;
    copyCtor: copyCtor;
    defaultCtor: defaultCtor;
    dtor: dtor;
    hasConstexprNonCopyMoveConstructor?: boolean;
    isAggregate?: boolean;
    isEmpty?: boolean;
    isLiteral?: boolean;
    isPOD?: boolean;
    isStandardLayout?: boolean;
    isTrivial?: boolean;
    isTriviallyCopyable?: boolean;
    moveAssign: moveAssign;
    moveCtor: moveCtor;
    canPassInRegisters?: boolean;
    hasUserDeclaredConstructor?: boolean;
    hasVariantMembers?: boolean;
    isPolymorphic?: boolean;
    hasMutableFields?: boolean;
    isGenericLambda?: boolean;
    isLambda?: boolean;
    isAbstract?: boolean;
}

export interface copyAssign {
    hasConstParam: boolean;
    implicitHasConstParam: boolean;
    needsImplicit?: boolean;
    simple?: boolean;
    trivial?: boolean;
    userDeclared?: boolean;
    needsOverloadResolution?: boolean;
    nonTrivial?: boolean;
}

export interface copyCtor {
    hasConstParam: boolean;
    implicitHasConstParam: boolean;
    needsImplicit?: boolean;
    simple?: boolean;
    trivial?: boolean;
    userDeclared?: boolean;
    needsOverloadResolution?: boolean;
    nonTrivial?: boolean;
}

export interface defaultCtor {
    defaultedIsConstexpr?: boolean;
    exists?: boolean;
    isConstexpr?: boolean;
    needsImplicit?: boolean;
    trivial?: boolean;
    nonTrivial?: boolean;
    userProvided?: boolean;
}

export interface dtor {
    irrelevant?: boolean;
    needsImplicit?: boolean;
    simple?: boolean;
    trivial?: boolean;
    nonTrivial?: boolean;
    userDeclared?: boolean;
    needsOverloadResolution?: boolean;
    id?: string;
    kind?: string;
    name?: string;
    type?: type;
}

export interface moveAssign {
    exists?: boolean;
    needsImplicit?: boolean;
    simple?: boolean;
    trivial?: boolean;
    userDeclared?: boolean;
    needsOverloadResolution?: boolean;
    nonTrivial?: boolean;
}

export interface moveCtor {
    exists?: boolean;
    needsImplicit?: boolean;
    simple?: boolean;
    trivial?: boolean;
    userDeclared?: boolean;
    needsOverloadResolution?: boolean;
    nonTrivial?: boolean;
}

export interface fixedUnderlyingType {
    qualType: string;
    desugaredQualType?: string;
    typeAliasDeclId?: string;
}

export interface spellingLoc {
    offset: number;
    file?: string;
    line?: number;
    col: number;
    tokLen: number;
    includedFrom?: includedFrom;
    presumedLine?: number;
}

export interface expansionLoc {
    offset: number;
    line?: number;
    col: number;
    tokLen: number;
    includedFrom: includedFrom;
    file?: string;
    isMacroArgExpansion?: boolean;
}

export interface ownedTagDecl {
    id: string;
    kind: string;
    name?: string;
}

export interface originalNamespace {
    id: string;
    kind: string;
    name: string;
}

export interface referencedDecl {
    id: string;
    kind: string;
    name?: string;
    type: type;
}

export interface target {
    id: string;
    kind: string;
    name: string;
    type?: type;
}

export interface argType {
    desugaredQualType?: string;
    qualType: string;
    typeAliasDeclId?: string;
}

export interface defaultArg {
    kind: string;
    type?: type;
    isExpr?: boolean;
    ['inherited from']?: InheritedFrom;
}

export interface bases {
    access: string;
    type: type;
    writtenAccess: string;
}

export interface lookups {
    id: string;
    kind: string;
    name: string;
    type?: type;
}

export interface foundReferencedDecl {
    id: string;
    kind: string;
    name: string;
}

export interface computeLHSType {
    qualType: string;
    desugaredQualType?: string;
    typeAliasDeclId?: string;
}

export interface computeResultType {
    qualType: string;
    desugaredQualType?: string;
    typeAliasDeclId?: string;
}

export interface anyInit {
    id: string;
    kind: string;
    name: string;
    type: type;
}

export interface ctorType {
    qualType: string;
    desugaredQualType?: string;
}

export interface baseInit {
    desugaredQualType?: string;
    qualType: string;
    typeAliasDeclId?: string;
}

export interface path {
    name: string;
}

export interface conversionFunc {
    id: string;
    kind: string;
    name: string;
    type: type;
}

export interface InheritedFrom {
    id: string;
    kind: string;
    name?: string;
    type?: type;
}

export interface operatorNewDecl {
    id: string;
    kind: string;
    name: string;
    type: type;
}

export interface operatorDeleteDecl {
    id: string;
    kind: string;
    name: string;
    type: type;
}

export interface array_filler {
    id: string;
    kind: string;
    range: range;
    type: type;
    valueCategory: string;
}



export type ClangASTD = inner;