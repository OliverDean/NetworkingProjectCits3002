var form = document.getElementById("question-form"); 

// Get the next and previous buttons
var nextButton = document.querySelector(".next-question");
var prevButton = document.querySelector(".prev-question");
document.cookie = "session_id=1yp5y1mq";



nextButton.addEventListener("click", function () {
  fetch(`http:/ClientBrowser/question_${id + 1}.html`, {
    method: "GET",
  })
    .then((response) => response.text())
    .then((data) => {
      // Load the next question
      loadQuestionScript(data);
    })
    .catch((error) => {
      console.error("Error:", error);
    });
});

prevButton.addEventListener("click", function () {
  fetch(`http:/ClientBrowser/question_${id - 1}.html`, {
    method: "GET",
  })
    .then((response) => response.text())
    .then((data) => {
      // Load the previous question
      loadQuestionScript(data);
    })
    .catch((error) => {
      console.error("Error:", error);
    });
});

window.onload = function () {
  // Extract the session id from the cookies
  var cookies = document.cookie.split(";");
  var sessionId = "";
  for (var i = 0; i < cookies.length; i++) {
    var cookie = cookies[i].trim();
    if (cookie.startsWith("session_id=")) {
      sessionId = cookie.substring("session_id=".length, cookie.length);
      break;
    }
  }
  if (sessionId === "") {
    console.error("Session id not found in the cookies");
    return;
  }

  // Use the session id to build the file name
  var fileName = "../" + sessionId + ".txt";

  var xhr = new XMLHttpRequest();
  xhr.open("GET", fileName, true);
  xhr.onreadystatechange = function () {
    if (xhr.readyState == 4 && xhr.status == 200) {
      var questions = parseQuestions(xhr.responseText); // Store the returned questions
      for (var i = 0; i < questions.length; i++) {
        var question = questions[i];
        loadQuestion(question.id, question.type);
      }
    }
  };
  xhr.send(null);
};

function parseQuestions(fileData) {
  var elements = fileData.split(";");
  var questions = [];
  var userName = elements.shift().trim(); // Get the user's name from the first element

  // Each question is defined by four elements
  while (elements.length >= 4) {
    var questionData = elements.splice(0, 4).map(e => e.trim()); // Get the next four elements and trim them
    if (questionData[0] === "q") {
      questions.push({
        type: questionData[1],
        id: questionData[2],
      });
    } else {
      console.error("Invalid question entry:", questionData);
    }
  }
  return questions;
}

function loadQuestion(id, type) {
  var fileName = `question_${id}.txt`;

  var xhr = new XMLHttpRequest();
  xhr.open("GET", fileName, true);
  xhr.onreadystatechange = function () {
    if (xhr.readyState == 4 && xhr.status == 200) {
      var questionData = xhr.responseText;
      loadQuestionScript(questionData, type, id);
    }
  };
  xhr.send(null);
}

function loadQuestionScript(questionData, type, id) {
  // Update the header with the question ID
  document.getElementById("question-header").textContent = `Question ID: ${id}`;
  // Based on the type, choose the function to call
  if (type === "python") {
    displayMCQuestion(questionData); // Call the multiple choice question function
  } else if (type === "programchallenge") {
    displayCodingQuestion(questionData); // Call the coding question function
  } else {
    console.error("Unknown question type:", type);
  }
}

function displayMCQuestion(questionData) {
  var lines = questionData.split("\n");
  var question = {
    id: lines[0],
    type: lines[1],
    text: lines[2],
    options: lines.slice(3, lines.length - 2),  // Exclude the answer line
    answer: lines[lines.length - 2],  // Update the line index for the answer
  };
  
  // Create a div for the question
  var questionDiv = document.createElement("div");
  questionDiv.className = "question-module multiple-choice";

  // Create a paragraph for the question text
  var questionText = document.createElement("p");
  questionText.textContent = question.text;
  questionDiv.appendChild(questionText);

  // Create radio buttons for each option
  for (var j = 0; j < question.options.length; j++) {
    var option = question.options[j];

    var optionDiv = document.createElement("div");
    optionDiv.className = "form-group";

    var input = document.createElement("input");
    input.type = "radio";
    input.id = "option" + (j + 1);
    input.name = "question1";
    input.value = option;
    optionDiv.appendChild(input);

    var label = document.createElement("label");
    label.for = input.id;
    label.textContent = option;
    optionDiv.appendChild(label);

    questionDiv.appendChild(optionDiv);
  }

  // Add the question div to the form
  form.appendChild(questionDiv);
}


function displayCodingQuestion(questionData) {
  var lines = questionData.split("\n");
  var question = {
    id: lines[0],
    type: lines[1],
    language: lines[2],
    text: lines[3],
    input: lines[4],
    output: lines[5],
  };
  var questionDiv = document.createElement("div");
  questionDiv.className = "question-module coding";

  var questionText = document.createElement("p");
  questionText.textContent = question.text;
  questionDiv.appendChild(questionText);

  if (question.type === "programchallenge") {
    var answerDiv = document.createElement("div");
    answerDiv.className = "form-group";

    var label = document.createElement("label");
    label.for = "codingAnswer";
    label.textContent = "Answer:";
    answerDiv.appendChild(label);

    var textarea = document.createElement("textarea");
    textarea.id = "codingAnswer";
    textarea.name = "codingAnswer";
    textarea.rows = "4";
    textarea.required = true;
    answerDiv.appendChild(textarea);

    questionDiv.appendChild(answerDiv);
  }

  form.appendChild(questionDiv);

  form.onsubmit = function (event) {
    event.preventDefault();
    var formData = new FormData(form);
    formData.append("questionNumber", question.id);

    var submitXhr = new XMLHttpRequest();
    submitXhr.open("POST", "http://your-server-url.com/submit-answer", true);
    submitXhr.onreadystatechange = function () {
      if (submitXhr.readyState == 4) {
        if (submitXhr.status == 200) {
          alert("Answer submitted successfully!");
        } else {
          alert("Error: " + submitXhr.statusText);
        }
      }
    };
    submitXhr.send(formData);
  };
}

function testDisplayQuestion() {
  var userCookieFile = `Joel;
  q;python;17;---;
  q;python;13;---;
  q;python;5;---;
  q;python;3;---;
  q;python;10;---;
  q;python;11;---;
  q;c;12;---;
  q;c;1;---;
  q;c;14;---;
  q;c;7;---;`;

  // Parse the questions from the userCookieFile
  var questions = parseQuestions(userCookieFile);

  // Find question 3
  var question3 = questions.find(question => question.id === "3");

  if (question3) {
    // Load and display question 3
    loadQuestion(question3.id, question3.type);
  } else {
    console.error("Question 3 not found");
  }
}

// Run the test function
testDisplayQuestion();
