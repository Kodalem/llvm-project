//===-- PatmosAsmParser.cpp - Parse Patmos assembly to MCInst instructions ----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "PatmosInstrInfo.h"
#include "InstPrinter/PatmosInstPrinter.h"
#include "MCTargetDesc/PatmosTargetStreamer.h"
#include "TargetInfo/PatmosTargetInfo.h"
#include "llvm/MC/MCParser/MCTargetAsmParser.h"
#include "llvm/MC/MCParser/MCParsedAsmOperand.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/MC/MCParser/AsmLexer.h"

using namespace llvm;

namespace {
struct PatmosOperand;

class PatmosAsmParser : public MCTargetAsmParser {
  MCAsmParser &Parser;
  const MCInstrInfo &MII;

  // Remember if we are inside of a bundle marker
  bool InBundle;

  // keep track of the bundle bit of the last instructions
  unsigned BundleCounter;

  unsigned IssueWidth;

  PrintBytesLevel ParseBytes;

  MCAsmParser &getParser() const { return Parser; }
  AsmLexer &getLexer() const { return Parser.getLexer(); }

  void Warning(SMLoc L, const Twine &Msg) { Parser.Warning(L, Msg); }
  bool Error(SMLoc L, const Twine &Msg) { return Parser.Error(L, Msg); }

  #define GET_ASSEMBLER_HEADER
  #include "PatmosGenAsmMatcher.inc"

public:
  PatmosAsmParser(const MCSubtargetInfo &sti, MCAsmParser &parser,
                  const MCInstrInfo &MII, const MCTargetOptions &Options)
    : MCTargetAsmParser(Options, sti, MII), Parser(parser), MII(MII),
      InBundle(false), BundleCounter(0)
  {
    IssueWidth = sti.getSchedModel().IssueWidth;

    switch (parser.getAssemblerDialect()) {
    case 0: ParseBytes = PrintAsEncoded; break;
    case 1: ParseBytes = PrintCallAsBytes; break;
    case 2: ParseBytes = PrintAllAsBytes; break;
    }
  }

  // NOTE: ParsePrefix was removed from MCTargetAsmParser in newer LLVM
  // versions (LLVM 13+). Prefix handling (bundle markers) is implemented
  // inline inside ParseInstruction below.

  bool parseInstruction(ParseInstructionInfo &Info, StringRef Name, SMLoc NameLoc, OperandVector &Operands) override;

  bool parseRegister(MCRegister &RegNo, SMLoc &StartLoc, SMLoc &EndLoc) override;
  bool parseRegister(MCRegister &RegNo, bool Required);

  bool ParseDirective(AsmToken DirectiveID) override;

  bool matchAndEmitInstruction(SMLoc IDLoc, unsigned &Opcode,
                               OperandVector &Operands, MCStreamer &Out,
                               uint64_t &ErrorInfo,
                               bool MatchingInlineAsm) override;

  ParseStatus tryParseRegister(MCRegister &Reg, SMLoc &StartLoc,
                               SMLoc &EndLoc) override;
  bool parseRegister(OperandVector &Operands, bool EmitError);

  void EatToEndOfStatement();

private:
  bool ParseOperand(OperandVector &Operands, unsigned OpNo);

  /// Parses the instruction guard, e.g. '(!$p1)', or produces the default instead.
  bool ParseGuard(SMLoc NameLoc, OperandVector &Operands);

  bool ParseMemoryOperand(OperandVector &Operands);

  /// ParsePredicateOperand - parse a predicate operand including an optional negate flag. Adds two
  /// operands.
  /// \param checkClass - if true, only add the flag operand if the register is a predicate register
  bool ParsePredicateOperand(OperandVector &Operands, bool checkClass = false);

  bool ParseImmediate(OperandVector &Operands);

  /// ParseToken - Check if the Lexer is currently over the given token kind, and add it as operand if so.
  bool ParseToken(OperandVector &Operands, AsmToken::TokenKind Kind);
  bool ParseInstruction(ParseInstructionInfo &Info, StringRef Name,
                        SMLoc NameLoc, OperandVector &Operands);

  /// isPredSrcOperand - Check whether the operand might be a predicate source
  /// operand (i.e., has a negate flag)
  bool isPredSrcOperand(StringRef Mnemonic, unsigned OpNo);

  bool ParseDirectiveWord(unsigned Size, SMLoc L);

  bool ParseDirectiveFStart(SMLoc L);
};

/// PatmosOperand - Instances of this class represent a parsed Patmos machine
/// instruction.
struct PatmosOperand : public MCParsedAsmOperand {
    enum KindTy {
      Token,
      Immediate,
      Register,
      Memory
    } Kind;

    SMLoc StartLoc, EndLoc;

    struct TokOp {
      const char *Data;
      unsigned Length;
    };

    struct RegOp {
      unsigned RegNum;
    };

    struct ImmOp {
      const MCExpr *Val;
    };

    struct MemOp {
      unsigned Base;
      const MCExpr *Off;
    };

    union {
      struct TokOp Tok;
      struct RegOp Reg;
      struct ImmOp Imm;
      struct MemOp Mem;
    };

    PatmosOperand(KindTy K) : MCParsedAsmOperand(), Kind(K) {}
  public:
    PatmosOperand(const PatmosOperand &o) : MCParsedAsmOperand() {
      Kind = o.Kind;
      StartLoc = o.StartLoc;
      EndLoc = o.EndLoc;
      switch (Kind) {
      case Register:
        Reg = o.Reg;
        break;
      case Immediate:
        Imm = o.Imm;
        break;
      case Token:
        Tok = o.Tok;
        break;
      case Memory:
        Mem = o.Mem;
        break;
      }
    }

    /// getStartLoc - Get the location of the first token of this operand.
    SMLoc getStartLoc() const { return StartLoc; }

    /// getEndLoc - Get the location of the last token of this operand.
    SMLoc getEndLoc() const { return EndLoc; }

    MCRegister getReg() const {
      assert(Kind == Register && "Invalid access!");
      return Reg.RegNum;
    }

    const MCExpr *getImm() const {
      assert(Kind == Immediate && "Invalid access!");
      return Imm.Val;
    }

    unsigned getMemBase() const {
      assert(Kind == Memory && "Invalid access!");
      return Mem.Base;
    }

    const MCExpr* getMemOff() const {
      assert(Kind == Memory && "Invalid access!");
      return Mem.Off;
    }

    bool isToken() const { return Kind == Token; }
    bool isImm() const { return Kind == Immediate; }
    bool isMem() const { return Kind == Memory; }
    bool isReg() const { return Kind == Register; }

    void addExpr(MCInst &Inst, const MCExpr *Expr) const {
      // Add as immediate when possible.  Null MCExpr = 0.
      if (Expr == 0)
        Inst.addOperand(MCOperand::createImm(0));
      else if (const MCConstantExpr *CE = dyn_cast<MCConstantExpr>(Expr))
        Inst.addOperand(MCOperand::createImm(CE->getValue()));
      else
        Inst.addOperand(MCOperand::createExpr(Expr));
    }

    void addRegOperands(MCInst &Inst, unsigned N) const {
      assert(N == 1 && "Invalid number of operands!");
      Inst.addOperand(MCOperand::createReg(getReg()));
    }

    void addImmOperands(MCInst &Inst, unsigned N) const {
      assert(N == 1 && "Invalid number of operands!");
      addExpr(Inst, getImm());
    }

    void addMemOperands(MCInst &Inst, unsigned N) const {
      assert(N == 2 && "Invalid number of operands!");

      Inst.addOperand(MCOperand::createReg(getMemBase()));

      addExpr(Inst, getMemOff());
    }

    StringRef getToken() const {
      assert(Kind == Token && "Invalid access!");
      return StringRef(Tok.Data, Tok.Length);
    }

  void print(raw_ostream &OS, const MCAsmInfo &MAI) const override;

    static PatmosOperand *CreateToken(StringRef Str, SMLoc S) {
      PatmosOperand *Op = new PatmosOperand(Token);
      Op->Tok.Data = Str.data();
      Op->Tok.Length = Str.size();
      Op->StartLoc = S;
      Op->EndLoc = S;
      return Op;
    }

    static PatmosOperand *CreateReg(unsigned RegNum, SMLoc S, SMLoc E) {
      PatmosOperand *Op = new PatmosOperand(Register);
      Op->Reg.RegNum = RegNum;
      Op->StartLoc = S;
      Op->EndLoc = E;
      return Op;
    }

    static PatmosOperand *CreateImm(const MCExpr *Val, SMLoc S, SMLoc E) {
      PatmosOperand *Op = new PatmosOperand(Immediate);
      Op->Imm.Val = Val;
      Op->StartLoc = S;
      Op->EndLoc = E;
      return Op;
    }

    static PatmosOperand *CreateConstant(int value, SMLoc S, SMLoc E, MCContext &Ctx) {
      PatmosOperand *Op = new PatmosOperand(Immediate);
      Op->Imm.Val = MCConstantExpr::create(value, Ctx);
      Op->StartLoc = S;
      Op->EndLoc = E;
      return Op;
    }

    static PatmosOperand *CreateFlag(bool flag, SMLoc S, SMLoc E, MCContext &Ctx) {
      return CreateConstant(flag ? 1 : 0, S, E, Ctx);
    }

    static PatmosOperand *CreateMem(unsigned Base, const MCExpr *Off, SMLoc S,
                                    SMLoc E) {
      PatmosOperand *Op = new PatmosOperand(Memory);
      Op->Mem.Base = Base;
      Op->Mem.Off = Off;
      Op->StartLoc = S;
      Op->EndLoc = E;
      return Op;
    }
};

} // end anonymous namespace.

void PatmosOperand::print(raw_ostream &OS, const MCAsmInfo &MAI) const {
  switch (Kind) {
  case Token:
    OS << "Token: " << getToken();
    break;
  case Register:
    // MCRegister supports << directly in LLVM 23
    OS << "Reg: " << getReg();
    break;
  case Immediate: {
    if (const MCExpr *E = getImm())
      MAI.printExpr(OS, *E);
    break;
  }
  case Memory: {
    OS << "Mem: " << getMemBase() << "+";
    if (const MCExpr *Off = getMemOff())
      MAI.printExpr(OS, *Off);
    break;
  }
  }
}


/// @name Auto-generated Match Functions
/// {

#define GET_REGISTER_MATCHER
#define GET_MATCHER_IMPLEMENTATION
#include "PatmosGenAsmMatcher.inc"

/// }


bool PatmosAsmParser::matchAndEmitInstruction(SMLoc IDLoc, unsigned &Opcode,
                        SmallVectorImpl<std::unique_ptr<MCParsedAsmOperand>> &Operands,
                        MCStreamer &Out, uint64_t &ErrorInfo,
			bool MatchingInlineAsm) {
  MCInst Inst;
  SMLoc ErrorLoc;

  switch (MatchInstructionImpl(Operands, Inst, ErrorInfo, MatchingInlineAsm, 0))
  {
  default: break;
  case Match_Success:
  {
    const MCInstrDesc &MID = MII.get(Inst.getOpcode());

    // The predicate operands must be the first input operands
    // i.e. after the output operands (also called definitions)
    auto insert_pred_at = MID.getNumDefs();
    auto insert_pred_flag_at = insert_pred_at + 1;

    // Add bundle marker
    Inst.addOperand(MCOperand::createImm(InBundle));

    // If we have an ALUi immediate instruction and the immediate does not fit
    // 12bit, use ALUl version of instruction
    uint64_t Format = MID.TSFlags & PatmosII::FormMask;
    unsigned ImmOpNo = getPatmosImmediateOpNo( MID.TSFlags );
    bool ImmSigned = isPatmosImmediateSigned( MID.TSFlags );
    bool HasImm = hasPatmosImmediate( MID.TSFlags );

    if (HasImm) {
      MCOperand &MCO = Inst.getOperand( ImmOpNo );

      // If we have an immediate, shift the value according to the asm dialect
      if (MCO.isImm() && (ParseBytes == PrintAllAsBytes ||
          (ParseBytes == PrintCallAsBytes && Inst.getOpcode() == Patmos::CALL)))
      {
        unsigned ImmShift = getPatmosImmediateShift( MID.TSFlags );
        MCO.setImm(MCO.getImm() / (1 << ImmShift));
      }
    }

    unsigned ALUlOpcode;
    if (Format == PatmosII::FrmALUi && HasImm) {
      MCOperand &MCO = Inst.getOperand( ImmOpNo );

      if (MCO.isExpr()) {
        if (HasALUlVariant(Inst.getOpcode(), ALUlOpcode)){
          if (InBundle) {
            return Error(IDLoc, "long immediate instruction cannot be in the second slot of a bundle");
          } else {
            // If we have an expression and can use ALUl, do so
            Inst.setOpcode(ALUlOpcode);
            // ALUl counts as two operations
            BundleCounter++;
          }
        }
      } else {
        assert(MCO.isImm() && "expected immediate operand for ALUi format");

        if (!isUInt<12>(MCO.getImm())) {
          if (isUInt<12>(-MCO.getImm()) && Inst.getOpcode() == Patmos::LIi) {
            // Make this an sub instead
            MCO.setImm(-MCO.getImm());
            Inst.setOpcode(Patmos::LIin);
          }
          else if (HasALUlVariant(Inst.getOpcode(), ALUlOpcode)) {
            if (InBundle || BundleCounter) {
              return Error(IDLoc, "immediate size requires ALUl but ALUl must "
                                  "not appear inside bundles");
            }
            Inst.setOpcode(ALUlOpcode);
            // ALUl counts as two operations
            BundleCounter++;

            if (!isInt<32>(MCO.getImm()) && !isUInt<32>(MCO.getImm())) {
              return Error(IDLoc,"immediate operand too large for ALUl format");
            }
          }
          else {
            return Error(IDLoc, "immediate operand too large for ALUi format "
                                "and ALUl is not available for this opcode");
          }
        }
      }
      if (BundleCounter > IssueWidth) {
        return Error(IDLoc, "operand size required ALUl instruction, but this "
                            "makes the bundle too large");
      }
    }
    else if (Format == PatmosII::FrmALUl) {
      if (InBundle || BundleCounter) {
        return Error(IDLoc, "ALUl instruction cannot be bundled");
      }
      // ALUl counts as two instructions
      BundleCounter++;

      MCOperand &MCO = Inst.getOperand( ImmOpNo );

      // No immediates larger than 32bit allowed ..
      if (MCO.isImm() && !isInt<32>(MCO.getImm()) && !isUInt<32>(MCO.getImm()))
      {
        return Error(IDLoc, "immediate operand too large for ALUl format");
      }
    }

    if (Format == PatmosII::FrmCFLi || Format == PatmosII::FrmSTCi) {
      const MCOperand &MCO = Inst.getOperand(ImmOpNo);
      if (!MCO.isExpr()) {
        assert(MCO.isImm() && "expected immediate operand for ALUi format");

        if ((Format == PatmosII::FrmSTCi && !isUInt<18>(MCO.getImm())) ||
            ( ImmSigned && !isInt<22>(MCO.getImm())) ||
            (!ImmSigned && !isUInt<22>(MCO.getImm()))) {
          return Error(IDLoc, "immediate operand is out of range");
        }
      }
    }

    if (Format == PatmosII::FrmSTT || Format == PatmosII::FrmLDT) {
      const MCOperand &MCO = Inst.getOperand(ImmOpNo);
      if (!MCO.isExpr()) {
        assert(MCO.isImm() && "expected immediate operand for ALUi format");

        if (( ImmSigned && !isInt<7>(MCO.getImm())) ||
            (!ImmSigned && !isUInt<7>(MCO.getImm()))) {
          return Error(IDLoc, "immediate offset is out of range");
        }
      }
    }

    if (BundleCounter >= IssueWidth) {
      Error(IDLoc, "the bundle consists of too many instructions");
      return true;
    }

    BundleCounter = InBundle ? BundleCounter + 1 : 0;

    Out.emitInstruction(Inst, getSTI());
    return false;
  }
  case Match_MissingFeature:
    return Error(IDLoc, "instruction use requires an option to be enabled");
  case Match_MnemonicFail:
      return Error(IDLoc, "unrecognized instruction mnemonic");
  case Match_InvalidOperand:
    ErrorLoc = IDLoc;
    if (ErrorInfo != ~0U) {
      if (ErrorInfo >= Operands.size())
        return Error(IDLoc, "too few operands for instruction");

      ErrorLoc = static_cast<PatmosOperand*>(&*Operands[ErrorInfo])->getStartLoc();
      if (ErrorLoc == SMLoc()) ErrorLoc = IDLoc;
    }

    return Error(ErrorLoc, "invalid operand for instruction or syntax mismatch");
  }

  llvm_unreachable("Implement any new match types added!");
}

ParseStatus PatmosAsmParser::tryParseRegister(MCRegister &RegNo, SMLoc &StartLoc, SMLoc &EndLoc) {
  // ParseRegister now typically returns bool (true on error)
  if (parseRegister(RegNo, StartLoc, EndLoc)) {
    return ParseStatus::Failure;
  }

  if (!RegNo.isValid()) { // Modern way to check for "no register"
    return ParseStatus::NoMatch;
  }

  return ParseStatus::Success;
}

bool PatmosAsmParser::
parseRegister(OperandVector &Operands, bool EmitError) {
  AsmLexer &Lexer = getLexer();
  SMLoc S = Lexer.getLoc();
  MCRegister RegNo; // Use MCRegister instead of unsigned

  // Modern ParseRegister usually takes (RegNo, StartLoc, EndLoc)
  SMLoc Start, End;
  if (parseRegister(RegNo, Start, End)) {
    return true;
  }

  if (!RegNo.isValid()) {
    return !EmitError || Error(S, "Missing register name");
  }

  // Use RegNo.id() to pass the raw integer to your CreateReg function
  Operands.push_back(std::unique_ptr<MCParsedAsmOperand>(
      PatmosOperand::CreateReg(RegNo.id(), S, End)));

  return false;
}

bool PatmosAsmParser::
parseRegister(MCRegister &RegNo, SMLoc &StartLoc, SMLoc &EndLoc) {
  StartLoc = getLexer().getLoc();
  if (parseRegister(RegNo, StartLoc, EndLoc)) {
    return true;
  }

  EndLoc = getLexer().getLoc();
  return false;
}

bool PatmosAsmParser::
parseRegister(MCRegister &RegNo, bool Required) {
  AsmLexer &Lexer = getLexer();

  if (Lexer.is(AsmToken::Dollar)) {
    Lexer.Lex(); // Consume '$'
  } else {
    return Required; // If not '$' and not required, return success (no reg found)
  }

  if (Lexer.is(AsmToken::Identifier)) {
    StringRef RegName = Lexer.getTok().getIdentifier();

    // Match the name and convert the result to MCRegister
    unsigned RegID = MatchRegisterName(RegName);

    if (!RegID) {
      RegID = StringSwitch<unsigned>(RegName)
        .Case("sl", Patmos::SL)
        .Case("sh", Patmos::SH)
        .Case("ss", Patmos::SS)
        .Case("st", Patmos::ST)
        .Case("srb", Patmos::SRB)
        .Case("sro", Patmos::SRO)
        .Case("sxb", Patmos::SXB)
        .Case("sxo", Patmos::SXO)
        .Default(0);
    }
    if (RegID == 0) {
      return Error(Lexer.getLoc(), "register name not valid");
    }

    RegNo = MCRegister::from(RegID); // Convert raw ID to MCRegister
    Lexer.Lex(); // Consume the identifier
    return false; // Success
  }

  return Error(Lexer.getLoc(), "register prefix $ is not followed by a register name");
}
bool PatmosAsmParser::
ParseMemoryOperand(OperandVector &Operands)  {
  AsmLexer &Lexer = getLexer();
  SMLoc RegStart, RegEnd;
  MCRegister RegNo; // Use MCRegister instead of unsigned

  if (ParseToken(Operands, AsmToken::LBrac)) {
    return true;
  }

  // try to match rN +/- Imm, rN, or Imm

  if (tryParseRegister(RegNo, RegStart, RegEnd).isSuccess()) {
    // If parsing succeeded, use the returned RegNo
    Operands.push_back(std::unique_ptr<PatmosOperand>(
        PatmosOperand::CreateReg(RegNo.id(), RegStart, RegEnd)));

  } else {

    if (Lexer.is(AsmToken::RBrac)) {
      // Default offset
      SMLoc E = Lexer.getLoc();
      Operands.push_back(std::unique_ptr<MCParsedAsmOperand>(
          PatmosOperand::CreateConstant(0, E, E, getParser().getContext())));

      return ParseToken(Operands, AsmToken::RBrac);

    } else if (Lexer.is(AsmToken::Plus)) {
      // lex away the plus symbol, leave a minus, fail on everything else
      Lexer.Lex();
    } else if (Lexer.isNot(AsmToken::Minus)) {
      return Error(Lexer.getLoc(), "invalid separator between register and offset");
    }
  }

  if (ParseImmediate(Operands)) {
    return true;
  }

  return ParseToken(Operands, AsmToken::RBrac);
}

bool PatmosAsmParser::
ParsePredicateOperand(OperandVector &Operands, bool checkClass) {
  AsmLexer &Lexer = getLexer();
  SMLoc StartLoc = Lexer.getLoc();
  MCRegister RegNo;
  SMLoc RegStart, RegEnd;

  bool flag = false;
  if (Lexer.is(AsmToken::Exclaim)) {
    flag = true;
    Lexer.Lex();
  }

  SMLoc RegLoc = Lexer.getLoc();

  // 1. Parse the register into our local variables
  if (parseRegister(RegNo, RegStart, RegEnd)) {
    return true;
  }

  // 2. Create the register operand and push it to the vector
  Operands.push_back(std::unique_ptr<MCParsedAsmOperand>(
      PatmosOperand::CreateReg(RegNo.id(), RegStart, RegEnd)));

  if (checkClass) {
    // 3. Check the name directly using the MCRegister object
    if (PatmosInstPrinter::getRegisterName(RegNo)[0] != 'p') {
      if (flag) {
        Error(StartLoc, "Negation of registers other than predicates is invalid.");
      }
      return false; // Not a predicate, but not necessarily a parse failure
    }
  }

  // 4. Push the Flag/Negation operand
  Operands.push_back(std::unique_ptr<MCParsedAsmOperand>(
      PatmosOperand::CreateFlag(flag, StartLoc, RegEnd, getParser().getContext())));


  return false;
}

bool PatmosAsmParser::
ParseOperand(OperandVector &Operands, unsigned OpNo)  {
  AsmLexer &Lexer = getLexer();

  // Handle all the various operand types here: Imm, reg, memory, predicate, label
  if (Lexer.is(AsmToken::LBrac)) {
    return ParseMemoryOperand(Operands);
  }
  if (Lexer.is(AsmToken::Exclaim)) {
    // we never allow a negated predicate as first out operand
    if (OpNo == 0) {
      return Error(Lexer.getLoc(), "destination predicate cannot be negated");
    }
    return ParsePredicateOperand(Operands);
  }
  if (Lexer.is(AsmToken::Dollar)) {

    StringRef Mnemonic = ((PatmosOperand*)&*Operands[0])->getToken();
    if (isPredSrcOperand(Mnemonic, OpNo)) {
      return ParsePredicateOperand(Operands, true);
    }

    return ParsePredicateOperand(Operands, false);
  }
  if (Lexer.is(AsmToken::Identifier)) {
    // Parse it as a label
    return ParseImmediate(Operands);
  }

  // Parse as immediate or some other form of symbolic expression
  return ParseImmediate(Operands);
}

bool PatmosAsmParser::ParseGuard(SMLoc NameLoc, OperandVector &Operands) {
  AsmLexer &Lexer = getLexer();

  if (Lexer.is(AsmToken::LParen)) {
    // If a guard is given, parse it
    Lexer.Lex();

    if (ParsePredicateOperand(Operands)) {
      return true;
    }

    if (Lexer.isNot(AsmToken::RParen)) {
      return true;
    }
    Lexer.Lex();

  } else {
    // If no guard is given, choose the default.
    Operands.push_back(std::unique_ptr<MCParsedAsmOperand>(
        PatmosOperand::CreateReg(Patmos::P0, NameLoc, NameLoc)));
    Operands.push_back(std::unique_ptr<MCParsedAsmOperand>(
        PatmosOperand::CreateFlag(false, NameLoc, NameLoc,
                                                 getParser().getContext())));
  }

  return false;
}

bool PatmosAsmParser::ParseImmediate(OperandVector &Operands) {
  AsmLexer &Lexer = getLexer();
  SMLoc S = Lexer.getLoc();

  const MCExpr *EVal;
  switch (Lexer.getKind()) {
  default: return true;
  case AsmToken::LParen:
  case AsmToken::Plus:
  case AsmToken::Minus:
  case AsmToken::Integer:
  case AsmToken::Identifier:
    SMLoc E;
    if (getParser().parseExpression(EVal, E))
      return true;

    Operands.push_back(std::unique_ptr<MCParsedAsmOperand>(
        PatmosOperand::CreateImm(EVal, S, E)));
    return false;
  }
}

bool PatmosAsmParser::ParseToken(OperandVector &Operands,
                                 AsmToken::TokenKind Kind)
{
  AsmLexer &Lexer = getLexer();

  if (Lexer.isNot(Kind)) {
    return Error(Lexer.getLoc(), "unexpected token");
  }

  Operands.push_back(std::unique_ptr<MCParsedAsmOperand>(
      PatmosOperand::CreateToken(Lexer.getTok().getString(), Lexer.getLoc())));
  Lexer.Lex();

  return false;
}


// ParsePrefix was removed from the base class. Its behavior (handling of
// bundle markers / prefixes) is now inlined into ParseInstruction.

bool PatmosAsmParser::ParseInstruction(ParseInstructionInfo &Info, StringRef Name, SMLoc NameLoc,
                 OperandVector &Operands)
{
  AsmLexer &Lexer = getLexer();

  // Handle legacy prefix/bundle markers that previously lived in
  // ParsePrefix (removed from the base class in LLVM 13+).
  if (Lexer.is(AsmToken::RCurly)) {
    // Try to recover..
    InBundle = false;
    BundleCounter = 0;
    // TODO we either need to go back to the previous instruction (but
    // that one might already be emitted and deleted!) or handle this earlier.
    if (Error(Lexer.getLoc(), "Closing bracket must appear immediately "
              "after the instruction (for now)."))
      return true;
  }

  // Check if we start a new bundle
  if (Name == "{" || Lexer.is(AsmToken::LCurly)) {
    if (InBundle) {
      return Error(Lexer.getLoc(), "previous bundle has not been closed.");
    }
    InBundle = true;
    if (Name != "{") {
      Lexer.Lex();
    }

    // Allow newline(s) following '{'
    while (Lexer.is(AsmToken::EndOfStatement) &&
           Lexer.getTok().getString() != ";") {
      // TODO accept # comments
      Lexer.Lex();
    }
  }
  if (Name == "{") {
    // The prefix has some tokens. Therefore, 'Name' doesn't contain
    // the mnemonic. We need it to do so.	
    if (Lexer.isNot(AsmToken::Identifier) && Lexer.isNot(AsmToken::String)) {
      return Error(Lexer.getLoc(), "Couldn't find Mnemonic");
    }
    Name = getTok().getIdentifier();
    NameLoc = Lexer.getLoc();
    Lex();
  }

  // The first operand is the token for the instruction name
  // We need to split at . in mnemonic names, this is the way the matcher
  // expects it.
  size_t Next = Name.find('.');
  StringRef Mnemonic = Name.slice(0, Next);

  Operands.insert(Operands.begin(),
      std::unique_ptr<MCParsedAsmOperand>(PatmosOperand::CreateToken(Mnemonic, NameLoc)));

  if (Next != StringRef::npos) {
    // there is a format/modifier token in mnemonic, add as first operand
    StringRef Format = Name.slice(Next, StringRef::npos);
    Operands.insert(Operands.begin() + 1,
        std::unique_ptr<MCParsedAsmOperand>(PatmosOperand::CreateToken(Format, NameLoc)));
  }

  ParseGuard(NameLoc, Operands);

  unsigned OpNo = 0;

  // If there are no more operands then finish
  while (Lexer.isNot(AsmToken::EndOfStatement)) {

    // last instruction in a bundle?
    if (Lexer.is(AsmToken::RCurly)) {
      if (!InBundle) {
        SMLoc TokLoc = Lexer.getLoc();
        EatToEndOfStatement();
        return Error(TokLoc, "found bundle end marker without a matching "
                             "start marker");
      }
      InBundle = false;
      Lexer.Lex();
      return false;
    }

    if (Lexer.is(AsmToken::Comma)) {
      // we do not start with a comma before any operands
      if (OpNo == 0) {
        SMLoc TokLoc = Lexer.getLoc();
        EatToEndOfStatement();
        return Error(TokLoc, "comma before first operand");
      }
      Lex();
    } else if (Lexer.is(AsmToken::Equal)) {
      // add it as a token for the matcher
      // TODO if somebody writes something like 'r1, r2, r3' instead of 'r1 = r2, r3', he will
      //      get a 'register type mismatch' error for 'r3', which is *very* confusing.
      if (ParseToken(Operands, AsmToken::Equal)) {
        EatToEndOfStatement();
        return true;
      }
    } else if (OpNo > 0) {
      // We need some separation between operands
      SMLoc TokLoc = Lexer.getLoc();
      EatToEndOfStatement();
      return Error(TokLoc, "missing separator between operands or instructions");
    }

    if (ParseOperand(Operands, OpNo)) {
      EatToEndOfStatement();
      return true;
    }

    OpNo++;
  }

  return false;
}

bool PatmosAsmParser::
ParseDirective(AsmToken DirectiveID) {
  StringRef IDVal = DirectiveID.getIdentifier();
  if (IDVal == ".word")
    return ParseDirectiveWord(4, DirectiveID.getLoc());
  if (IDVal == ".half" || IDVal == ".hword")
    return ParseDirectiveWord(2, DirectiveID.getLoc());
  if (IDVal == ".fstart")
    return ParseDirectiveFStart(DirectiveID.getLoc());
  return true;
}

/// ParseDirectiveWord
///  ::= .word [ expression (, expression)* ]
bool PatmosAsmParser::ParseDirectiveWord(unsigned Size, SMLoc L) {
  if (getLexer().isNot(AsmToken::EndOfStatement)) {
    for (;;) {
      const MCExpr *Value;
      SMLoc E;
      if (getParser().parseExpression(Value, E))
        return true;

      getParser().getStreamer().emitValue(Value, Size);

      if (getLexer().is(AsmToken::EndOfStatement))
        break;

      // FIXME: Improve diagnostic.
      if (getLexer().isNot(AsmToken::Comma))
        return Error(L, "unexpected token in directive");
      Parser.Lex();
    }
  }

  Parser.Lex();
  return false;
}

/// ParseDirectiveFStart
///  ::= .fstart [ symbol , length, align ]
bool PatmosAsmParser::ParseDirectiveFStart(SMLoc L) {
  if (getLexer().is(AsmToken::EndOfStatement)) {
    return Error(L, "missing arguments to .fstart directive");
  }

  const MCSymbol *Start;
  const MCExpr *StartExpr;
  SMLoc E;
  if (getParser().parseExpression(StartExpr, E)) {
    return true;
  }
  if (StartExpr->getKind() == MCExpr::SymbolRef) {
    const MCSymbolRefExpr *SymRef = dyn_cast<MCSymbolRefExpr>(StartExpr);
    Start = &SymRef->getSymbol();
  } else {
    return Error(L, "first parameter of this directive must be a symbol name");
  }

  if (getLexer().isNot(AsmToken::Comma))
    return Error(L, "unexpected token in directive");
  Parser.Lex();

  const MCExpr *Length;
  if (getParser().parseExpression(Length, E)) {
    return true;
  }

  if (getLexer().isNot(AsmToken::Comma))
    return Error(L, "unexpected token in directive");
  Parser.Lex();

  int64_t align;
  if (getParser().parseAbsoluteExpression(align)) {
    return true;
  }
  if (align < 0) {
    return Error(L, "alignment value must be a positive value");
  }

  if (getLexer().isNot(AsmToken::EndOfStatement)) {
    return Error(L, "unexpected token in directive");
  }
  Parser.Lex();

  PatmosTargetStreamer *PTS = static_cast<PatmosTargetStreamer*>(
                                 getParser().getStreamer().getTargetStreamer());

  PTS->emitFStart(Start, Length, Align(align));

  return false;
}

bool PatmosAsmParser::isPredSrcOperand(StringRef Mnemonic, unsigned OpNo)
{
  // only src operands, only combine ops
  if (OpNo == 0) return false;

  // We check if the src op is actually a predicate register later in the
  // parse method
  if (Mnemonic == "por"  || Mnemonic == "pand" ||
      Mnemonic == "pxor") return true;
  if (Mnemonic == "pmov" || Mnemonic == "pnot" ||
      Mnemonic == "pset" || Mnemonic == "pclr") return true;
  // Note that mov might actually move between predicate and registers
  // (in the future)
  if (Mnemonic == "mov") return true;

  return false;
}

void PatmosAsmParser::EatToEndOfStatement() {
  AsmLexer &Lexer = getLexer();
  while (Lexer.isNot(AsmToken::EndOfStatement) &&
         Lexer.isNot(AsmToken::LCurly) &&
         Lexer.isNot(AsmToken::Eof)) {
    Lexer.Lex();
  }
}


extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializePatmosAsmParser() {
  RegisterMCAsmParser<PatmosAsmParser> X(getThePatmosTarget());
}

