
import { infra, win32 } from "mshell";
import { memo, useEffect, useState } from "react";


export const TestComponent = () => {
    const [num, setNum] = useState(0);

    return (
        <flex horizontal={true} padding={10} backgroundColor="#000000ff">
            <Calculator />
        </flex>
    );
}

export const Calculator = () => {
    const [display, setDisplay] = useState('0');
    const [previousValue, setPreviousValue] = useState<number | null>(null);
    const [operation, setOperation] = useState<string | null>(null);
    const [waitingForOperand, setWaitingForOperand] = useState(false);

    const inputNumber = (num: string) => {
        if (waitingForOperand) {
            setDisplay(num);
            setWaitingForOperand(false);
        } else {
            setDisplay(display === '0' ? num : display + num);
        }
    };

    const inputOperation = (nextOperation: string) => {
        const inputValue = parseFloat(display);

        if (previousValue === null) {
            setPreviousValue(inputValue);
        } else if (operation) {
            const currentValue = previousValue || 0;
            const newValue = calculate(currentValue, inputValue, operation);

            setDisplay(String(newValue));
            setPreviousValue(newValue);
        }

        setWaitingForOperand(true);
        setOperation(nextOperation);
    };

    const calculate = (firstValue: number, secondValue: number, operation: string) => {
        switch (operation) {
            case '+':
                return firstValue + secondValue;
            case '-':
                return firstValue - secondValue;
            case '×':
                return firstValue * secondValue;
            case '÷':
                return firstValue / secondValue;
            case '=':
                return secondValue;
            default:
                return secondValue;
        }
    };

    const performCalculation = () => {
        const inputValue = parseFloat(display);

        if (previousValue !== null && operation) {
            const newValue = calculate(previousValue, inputValue, operation);
            setDisplay(String(newValue));
            setPreviousValue(null);
            setOperation(null);
            setWaitingForOperand(true);
        }
    };

    const clear = () => {
        setDisplay('0');
        setPreviousValue(null);
        setOperation(null);
        setWaitingForOperand(false);
    };

    const Button = memo(({ onClick, backgroundColor = '#E3F2FD', textColor = '#1976D2', children, isOperator = false }: {
        onClick: () => void;
        backgroundColor?: string;
        textColor?: string;
        children: string;
        isOperator?: boolean;
    }) => (
        <flex
            onClick={onClick}
            backgroundColor={backgroundColor}
            borderRadius={30}
            paddingLeft={20}
            paddingRight={20}
            paddingTop={10}
            paddingBottom={10}
            onMouseEnter={() => { }}
        >
            <text
                text={children}
                fontSize={isOperator ? 24 : 20}
                color={textColor}
            />
        </flex>
    ));

    return (
        <flex backgroundColor="#F5F5F5" borderRadius={24} padding={20}>
            <flex backgroundColor="#FFFFFF" borderRadius={16} padding={20} paddingBottom={10}>
                <text
                    text={display}
                    fontSize={36}
                    color="#212121"
                />
            </flex>

            <flex padding={10}>
                <flex horizontal={true}>
                    <Button onClick={clear} backgroundColor="#FFCDD2" textColor="#D32F2F">C</Button>
                    <Button onClick={() => { }} backgroundColor="#E8F5E8" textColor="#388E3C">±</Button>
                    <Button onClick={() => { }} backgroundColor="#E8F5E8" textColor="#388E3C">%</Button>
                    <Button onClick={() => inputOperation('÷')} backgroundColor="#E3F2FD" textColor="#1976D2" isOperator>÷</Button>
                </flex>

                <flex horizontal={true}>
                    <Button onClick={() => inputNumber('7')}>7</Button>
                    <Button onClick={() => inputNumber('8')}>8</Button>
                    <Button onClick={() => inputNumber('9')}>9</Button>
                    <Button onClick={() => inputOperation('×')} backgroundColor="#E3F2FD" textColor="#1976D2" isOperator>×</Button>
                </flex>

                <flex horizontal={true}>
                    <Button onClick={() => inputNumber('4')}>4</Button>
                    <Button onClick={() => inputNumber('5')}>5</Button>
                    <Button onClick={() => inputNumber('6')}>6</Button>
                    <Button onClick={() => inputOperation('-')} backgroundColor="#E3F2FD" textColor="#1976D2" isOperator>-</Button>
                </flex>

                <flex horizontal={true}>
                    <Button onClick={() => inputNumber('1')}>1</Button>
                    <Button onClick={() => inputNumber('2')}>2</Button>
                    <Button onClick={() => inputNumber('3')}>3</Button>
                    <Button onClick={() => inputOperation('+')} backgroundColor="#E3F2FD" textColor="#1976D2" isOperator>+</Button>
                </flex>

                <flex horizontal={true}>
                    <Button onClick={() => inputNumber('0')}>0</Button>
                    <Button onClick={() => inputNumber('.')}>.</Button>
                    <Button onClick={performCalculation} backgroundColor="#4CAF50" textColor="#FFFFFF" isOperator>=</Button>
                </flex>
            </flex>
        </flex>
    );
};
// export const Calendar = () => {
//     const [currentDate, setCurrentDate] = useState(new Date());
//     const [selectedDate, setSelectedDate] = useState<Date | null>(null);

//     const monthNames = [
//         'January', 'February', 'March', 'April', 'May', 'June',
//         'July', 'August', 'September', 'October', 'November', 'December'
//     ];

//     const dayNames = ['Sun', 'Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat'];

//     const getDaysInMonth = (date: Date) => {
//         const year = date.getFullYear();
//         const month = date.getMonth();
//         const firstDay = new Date(year, month, 1);
//         const lastDay = new Date(year, month + 1, 0);
//         const firstDayOfWeek = firstDay.getDay();
//         const daysInMonth = lastDay.getDate();

//         const days = [];

//         // Add empty cells for days before the first day of the month
//         for (let i = 0; i < firstDayOfWeek; i++) {
//             days.push(null);
//         }

//         // Add all days of the month
//         for (let day = 1; day <= daysInMonth; day++) {
//             days.push(new Date(year, month, day));
//         }

//         return days;
//     };

//     const navigateMonth = (direction: number) => {
//         const newDate = new Date(currentDate);
//         newDate.setMonth(currentDate.getMonth() + direction);
//         setCurrentDate(newDate);
//     };

//     const isToday = (date: Date) => {
//         const today = new Date();
//         return date.toDateString() === today.toDateString();
//     };

//     const isSelected = (date: Date) => {
//         return selectedDate && date.toDateString() === selectedDate.toDateString();
//     };

//     const CalendarButton = ({ onClick, children, isToday = false, isSelected = false }: {
//         onClick: () => void;
//         children: string;
//         isToday?: boolean;
//         isSelected?: boolean;
//     }) => (
//         <flex
//             onClick={onClick}
//             backgroundColor={isSelected ? "#0078D4" : isToday ? "#E3F2FD" : "#FFFFFF"}
//             borderColor="#0078D4"
//             borderWidth={1}
//             padding={8}
//             onMouseEnter={() => {}}
//         >
//             <text
//                 text={children}
//                 fontSize={14}
//                 color={isSelected ? "#FFFFFF" : isToday ? "#0078D4" : "#323130"}
//             />
//         </flex>
//     );

//     const HeaderButton = ({ onClick, children }: {
//         onClick: () => void;
//         children: string;
//     }) => (
//         <flex
//             onClick={onClick}
//             backgroundColor="#F3F2F1"
//             borderColor="#0078D4"
//             borderWidth={1}
//             padding={8}
//             onMouseEnter={() => {}}
//         >
//             <text
//                 text={children}
//                 fontSize={16}
//                 color="#0078D4"
//             />
//         </flex>
//     );

//     const days = getDaysInMonth(currentDate);

//     return (
//         <flex backgroundColor="#FFFFFF" borderColor="#0078D4" borderWidth={2} padding={16}>
//             {/* Header */}
//             <flex horizontal={true} backgroundColor="#F8F9FA" padding={12} borderColor="#0078D4" borderWidth={1}>
//                 <HeaderButton onClick={() => navigateMonth(-1)}>‹</HeaderButton>
//                 <flex padding={8}>
//                     <text
//                         text={`${monthNames[currentDate.getMonth()]} ${currentDate.getFullYear()}`}
//                         fontSize={18}
//                         color="#323130"
//                     />
//                 </flex>
//                 <HeaderButton onClick={() => navigateMonth(1)}>›</HeaderButton>
//             </flex>

//             {/* Day names header */}
//             <flex horizontal={true} backgroundColor="#F3F2F1" borderColor="#0078D4" borderWidth={1}>
//                 {dayNames.map(day => (
//                     <flex key={day} padding={8} borderColor="#E1DFDD" borderWidth={1}>
//                         <text
//                             text={day}
//                             fontSize={12}
//                             color="#605E5C"
//                         />
//                     </flex>
//                 ))}
//             </flex>

//             {/* Calendar grid */}
//             <flex>
//                 {Array.from({ length: Math.ceil(days.length / 7) }, (_, weekIndex) => (
//                     <flex key={weekIndex} horizontal={true}>
//                         {days.slice(weekIndex * 7, (weekIndex + 1) * 7).map((date, dayIndex) => (
//                             <flex key={dayIndex} borderColor="#E1DFDD" borderWidth={1}>
//                                 {date ? (
//                                     <CalendarButton
//                                         onClick={() => setSelectedDate(date)}
//                                         isToday={isToday(date)}
//                                         isSelected={isSelected(date)}
//                                     >
//                                         {date.getDate().toString()}
//                                     </CalendarButton>
//                                 ) : (
//                                     <flex padding={8}>
//                                         <text text="" fontSize={14} color="#FFFFFF" />
//                                     </flex>
//                                 )}
//                             </flex>
//                         ))}
//                     </flex>
//                 ))}
//             </flex>

//             {/* Selected date display */}
//             {selectedDate && (
//                 <flex backgroundColor="#F3F2F1" padding={12} borderColor="#0078D4" borderWidth={1}>
//                     <text
//                         text={`Selected: ${selectedDate.toLocaleDateString()}`}
//                         fontSize={14}
//                         color="#323130"
//                     />
//                 </flex>
//             )}
//         </flex>
//     );
// };

const setInterval = infra.setInterval;
const clearInterval = infra.clearInterval;

export const PongGame = () => {
    const [gameState, setGameState] = useState({
        ballX: 400,
        ballY: 300,
        ballVelX: 4,
        ballVelY: 4,
        paddleY: 250,
        score: 0,
        gameOver: false
    });

    const GAME_WIDTH = 100;
    const GAME_HEIGHT = 300;
    const PADDLE_HEIGHT = 100;
    const PADDLE_WIDTH = 20;
    const BALL_SIZE = 20;
    const PADDLE_SPEED = 8;

    useEffect(() => {
        const gameLoop = setInterval(() => {
            setGameState(prev => {
                if (prev.gameOver) return prev;

                let newBallX = prev.ballX + prev.ballVelX;
                let newBallY = prev.ballY + prev.ballVelY;
                let newBallVelX = prev.ballVelX;
                let newBallVelY = prev.ballVelY;
                let newScore = prev.score;
                let newGameOver = false;

                // Ball collision with top and bottom walls
                if (newBallY <= 0 || newBallY >= GAME_HEIGHT - BALL_SIZE) {
                    newBallVelY = -newBallVelY;
                }

                // Ball collision with left wall (game over)
                if (newBallX <= 0) {
                    newGameOver = true;
                }

                // Ball collision with right wall
                if (newBallX >= GAME_WIDTH - BALL_SIZE) {
                    newBallVelX = -newBallVelX;
                    newScore += 1;
                }

                // Ball collision with paddle
                if (newBallX <= PADDLE_WIDTH &&
                    newBallY + BALL_SIZE >= prev.paddleY &&
                    newBallY <= prev.paddleY + PADDLE_HEIGHT) {
                    newBallVelX = Math.abs(newBallVelX);
                    // Add some angle based on where ball hits paddle
                    const hitPos = (newBallY + BALL_SIZE / 2 - prev.paddleY) / PADDLE_HEIGHT;
                    newBallVelY = (hitPos - 0.5) * 8;
                }

                return {
                    ballX: newBallX,
                    ballY: newBallY,
                    ballVelX: newBallVelX,
                    ballVelY: newBallVelY,
                    paddleY: prev.paddleY,
                    score: newScore,
                    gameOver: newGameOver
                };
            });
        }, 16);

        return () => clearInterval(gameLoop);
    }, []);

    useEffect(() => {
        const keyLoop = setInterval(() => {
            setGameState(prev => {
                let newPaddleY = prev.paddleY;

                if (win32.is_key_down('w') || win32.is_key_down('ArrowUp')) {
                    newPaddleY = Math.max(0, prev.paddleY - PADDLE_SPEED);
                }
                if (win32.is_key_down('s') || win32.is_key_down('ArrowDown')) {
                    newPaddleY = Math.min(GAME_HEIGHT - PADDLE_HEIGHT, prev.paddleY + PADDLE_SPEED);
                }

                return { ...prev, paddleY: newPaddleY };
            });
        }, 16);

        return () => clearInterval(keyLoop);
    }, []);

    const resetGame = () => {
        setGameState({
            ballX: 400,
            ballY: 300,
            ballVelX: 4,
            ballVelY: 4,
            paddleY: 250,
            score: 0,
            gameOver: false
        });
    };

    return (
        <flex backgroundColor="#F3F2F1" borderRadius={12} padding={20}>
            {/* Header */}
            <flex horizontal={true} backgroundColor="#FFFFFF" borderRadius={8} padding={16} borderColor="#0078D4" borderWidth={1}>
                <flex>
                    <text text={`Score: ${gameState.score}`} fontSize={24} color="#323130" />
                </flex>
                <flex>
                    <text text="Use ↑ ↓ Arrow Keys" fontSize={16} color="#605E5C" />
                </flex>
            </flex>

            {/* Game Area */}
            <flex
                backgroundColor="#000000"
                borderColor="#0078D4"
                borderWidth={2}
                borderRadius={8}
                paddingLeft={GAME_WIDTH}
                paddingTop={GAME_HEIGHT}
            >
                {/* Paddle */}
                <flex paddingLeft={gameState.paddleY}
                    paddingTop={10}>
                    <text
                        text="■■■■■■■■■■"
                        fontSize={12}
                        color="#FFFFFF"

                    />
                </flex>

                {/* Ball */}
                <flex paddingLeft={gameState.ballX}
                    paddingTop={gameState.ballY - 10}>
                    <text
                        text="⚪"
                        fontSize={16}
                        color="#FFFFFF"

                    />
                </flex>
            </flex>

            {/* Game Over Screen */}
            {gameState.gameOver && (
                <flex backgroundColor="#FFEBEE" borderColor="#D32F2F" borderWidth={2} borderRadius={8} padding={20}>
                    <flex backgroundColor="#FFFFFF" borderRadius={8} padding={20}>
                        <text text="Game Over!" fontSize={32} color="#D32F2F" />
                    </flex>
                    <flex backgroundColor="#FFFFFF" borderRadius={8} padding={16}>
                        <text text={`Final Score: ${gameState.score}`} fontSize={20} color="#323130" />
                    </flex>
                    <flex
                        onClick={resetGame}
                        backgroundColor="#0078D4"
                        borderRadius={8}
                        padding={16}
                        onMouseEnter={() => { }}
                    >
                        <text text="Play Again" fontSize={18} color="#FFFFFF" />
                    </flex>
                </flex>
            )}

            {/* Instructions */}
            <flex backgroundColor="#F8F9FA" borderRadius={8} padding={16} borderColor="#E1DFDD" borderWidth={1}>
                <text text="• Use ↑ ↓ arrow keys to move paddle" fontSize={14} color="#605E5C" />
                <text text="• Don't let the ball hit the left wall!" fontSize={14} color="#605E5C" />
                <text text="• Score points by hitting the right wall" fontSize={14} color="#605E5C" />
            </flex>
        </flex>
    );
};
