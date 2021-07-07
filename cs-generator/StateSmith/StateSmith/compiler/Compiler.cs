﻿using StateSmith.Input.antlr4;
using StateSmith.Input.Expansions;
using StateSmith.Input.Yed;
using System;
using System.Collections.Generic;
using System.Text;

namespace StateSmith.Compiler
{
    public class Compiler
    {
        public List<Vertex> rootVertices = new List<Vertex>();
        private Dictionary<Input.DiagramNode, Vertex> diagramVertexMap = new Dictionary<Input.DiagramNode, Vertex>();

        public void CompileFile(string filepath)
        {
            YedParser yedParser = new YedParser();
            Expander expander = new Expander(); //FIXME use for nodes and edges

            yedParser.Parse(filepath);

            foreach (var node in yedParser.GetRootNodes())
            {
                rootVertices.Add(ProcessNode(node, parentVertex: null));
            }

            foreach (var edge in yedParser.GetEdges())
            {
                ProcessEdge(edge);
            }
        }

        private void ProcessEdge(Input.DiagramEdge edge)
        {
            var sourceVertex = diagramVertexMap[edge.source];
            var targetVertex = diagramVertexMap[edge.target];

            LabelParser labelParser = new LabelParser();
            List<NodeBehavior> nodeBehaviors = labelParser.ParseEdgeLabel(edge.label);

            foreach (var nodeBehavior in nodeBehaviors)
            {
                var behavior = ConvertBehavior(nodeBehavior);
                behavior.transitionTarget = targetVertex;
                sourceVertex.behaviors.Add(behavior);

                //FIXME I believe this code will fail if there is an edge to an unrecognized diagram node like an image.
            }
        }

        private Vertex ProcessNode(Input.DiagramNode diagramNode, Vertex parentVertex)
        {
            if (diagramNode.label == null || diagramNode.label.Trim() == "")
            {
                return null;
            }

            LabelParser labelParser = new LabelParser();
            Node node = labelParser.ParseNodeLabel(diagramNode.label);
            Vertex thisVertex;

            switch (node)
            {
                default:
                    throw new ArgumentException("Unknown node: " + node);

                case NotesNode notesNode:
                    {
                        var noteVertex = new NotesVertex();
                        noteVertex.notes = notesNode.notes;
                        thisVertex = noteVertex;
                        break;
                    }

                case StateMachineNode stateMachineNode:
                    {
                        var sm = new Statemachine();
                        sm.name = stateMachineNode.name;
                        sm.nameIsGloballyUnique = true;
                        thisVertex = sm;
                        break;
                    }

                case StateNode stateNode:
                    {
                        if (stateNode is OrthoStateNode orthoStateNode)
                        {
                            var orthoState = new OrthoState();
                            thisVertex = orthoState;
                            orthoState.order = Double.Parse(orthoStateNode.order);
                            SetStateFromStateNode(stateNode, orthoState);
                        }
                        else
                        {
                            if (string.Equals(stateNode.stateName, "$initial_state", StringComparison.OrdinalIgnoreCase))
                            {
                                thisVertex = new InitialState();
                            }
                            else
                            {
                                var state = new State();
                                thisVertex = state;
                                SetStateFromStateNode(stateNode, state);
                            }
                        }

                        ConvertBehaviors(thisVertex, stateNode);
                        break;
                    }
            }

            thisVertex.yedId = diagramNode.id;
            diagramVertexMap.Add(diagramNode, thisVertex);

            if (parentVertex != null)
            {
                thisVertex.parent = parentVertex;
                parentVertex.children.Add(thisVertex);
            }

            foreach (var child in diagramNode.children)
            {
                ProcessNode(child, thisVertex);
            }

            return thisVertex;
        }

        private void ConvertBehaviors(Vertex vertex, StateNode stateNode)
        {
            foreach (var nodeBehavior in stateNode.behaviors)
            {
                Behavior behavior = ConvertBehavior(nodeBehavior);

                vertex.behaviors.Add(behavior);
            }
        }

        private static Behavior ConvertBehavior(NodeBehavior nodeBehavior)
        {
            var behavior = new Behavior
            {
                actionCode = nodeBehavior.actionCode,
                guardCode = nodeBehavior.guardCode,
                triggers = nodeBehavior.triggers
            };

            if (nodeBehavior.order != null)
            {
                behavior.order = Double.Parse(nodeBehavior.order);
            }

            return behavior;
        }

        private static void SetStateFromStateNode(StateNode stateNode, State state)
        {
            state.name = stateNode.stateName;
            state.nameIsGloballyUnique = stateNode.stateNameIsGlobal;
        }
    }
}
